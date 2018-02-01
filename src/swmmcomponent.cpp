#include "stdafx.h"

#include <QDebug>
#include <QDir>
#include <cstring>
#include <QString>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "core/idbasedexchangeitems.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/idbasedexchangeitems.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/unit.h"
#include "swmm5.h"
#include "headers.h"
#include "dataexchangecache.h"
#include "swmmcomponent.h"
#include "swmmcomponentinfo.h"
#include "swmmtimeseriesexchangeitems.h"
#include "core/argument1d.h"
#include "temporal/timedata.h"
#include "core/idbasedargument.h"
#include "spatial/geometryexchangeitems.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "progresschecker.h"


#include "pondedareainput.h"
#include "nodesurfaceflowoutput.h"
#include "nodewseinput.h"
#include "nodepondeddepthinput.h"
#include "linkflowoutput.h"
#include "spatial/geometryfactory.h"


using namespace std;
using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

SWMMComponent::SWMMComponent(const QString &id, SWMMComponentInfo *parent)
  :AbstractTimeModelComponent(id,parent),
    m_inputFilesArgument(nullptr),
    m_SWMMProject(nullptr),
    m_currentProgress(0)
{
  m_startDateTime = new SDKTemporal::DateTime(this);
  m_endDateTime = new SDKTemporal::DateTime(this);
  m_currentDateTime = new SDKTemporal::DateTime(this);

  createDimensions();
  createArguments();
}

SWMMComponent::SWMMComponent(const QString &id, const QString &caption, SWMMComponentInfo *parent)
  :AbstractTimeModelComponent(id, caption, parent),
    m_inputFilesArgument(nullptr),
    m_SWMMProject(nullptr),
    m_currentProgress(0)
{
  m_startDateTime = new SDKTemporal::DateTime(this);
  m_endDateTime = new SDKTemporal::DateTime(this);
  m_currentDateTime = new SDKTemporal::DateTime(this);

  createDimensions();
  createArguments();
}

SWMMComponent::~SWMMComponent()
{
  disposeProject();
  printf("");
}

QList<QString> SWMMComponent::validate()
{
  if(isInitialized())
  {
    setStatus(IModelComponent::Validating,"Validating...");

    //check connections

    setStatus(IModelComponent::Valid,"");
  }
  else
  {
    //throw has not been initialized yet.
  }

  return QList<QString>();
}

void SWMMComponent::prepare()
{

  if(!isPrepared() && isInitialized() && m_SWMMProject)
  {
    setStatus(IModelComponent::Preparing , "Preparing model..");

    for(auto output :  outputsInternal())
    {
      for(auto adaptedOutput : output->adaptedOutputs())
      {
        adaptedOutput->initialize();
      }
    }


    if (m_SWMMProject->ErrorCode)
    {
      QString message;
      hasError(message);
      setStatus(IModelComponent::Failed , message);
      return;
    }

    QString inputFilePath = (*m_inputFilesArgument)["Input File"];
    QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

    if(inputFile.exists())
    {

      if(m_scratchFileIO.isOpen())
        m_scratchFileIO.close();

      QString scratchFile = inputFile.absoluteFilePath().replace(inputFile.suffix(), "scratch");
      m_scratchFileIO.setFileName(scratchFile);

      if(m_scratchFileIO.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
        m_scratchFileTextStream.setDevice(&m_scratchFileIO);
        m_scratchFileTextStream.setRealNumberPrecision(10);
        m_scratchFileTextStream << "IDStartPos" << "\t" << m_SWMMProject->IDStartPos << endl;
        m_scratchFileTextStream << "InputStartPos" << "\t" << m_SWMMProject->InputStartPos << endl;
        m_scratchFileTextStream << "OutputStartPos" << "\t" << m_SWMMProject->OutputStartPos << endl;
        m_scratchFileTextStream.flush();
      }
    }

    intializeSurfaceInflows();
    updateOutputValues(QList<HydroCouple::IOutput*>());

    setStatus(IModelComponent::Updated ,"Finished preparing model");
    setPrepared(true);
  }
  else
  {
    setPrepared(false);
    setStatus(IModelComponent::Failed ,"Error occured when preparing model");
  }
}

void SWMMComponent::update(const QList<HydroCouple::IOutput *> &requiredOutputs)
{
  if(status() == IModelComponent::Updated)
  {

    clearDataCache(m_SWMMProject);
    resetSurfaceInflows();

    applyInputValues();

    DateTime elapsedTime = 0;
    swmm_step(m_SWMMProject, &elapsedTime);

    m_timeStep = elapsedTime * 86400.0;

    int y, m, d, h, mm, s;
    DateTime currentDateTime = m_SWMMProject->StartDateTime + m_SWMMProject->ElapsedTime;
    datetime_decodeDate(currentDateTime, &y,&m,&d);
    datetime_decodeTime(currentDateTime, &h,&mm,&s);

    m_currentDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));
    currentDateTimeInternal()->setModifiedJulianDay(m_currentDateTime->modifiedJulianDay());
    updateOutputValues(QList<IOutput*>({}));

    QString errMessage;

    //    if(m_scratchFileIO.isOpen() && m_SWMMProject->Nperiods > m_currentNPeriods)
    //    {
    //      m_currentNPeriods = m_SWMMProject->Nperiods;
    //      m_scratchFileTextStream << "NPeriods" << "\t" << m_currentNPeriods << endl;
    //      m_scratchFileTextStream.flush();
    //    }

    if(elapsedTime <= 0.0 && !m_SWMMProject->ErrorCode)
    {
      setStatus(IModelComponent::Done , "Simulation finished successfully|  DateTime: " + m_currentDateTime->dateTime().toString(Qt::ISODate),100);
    }
    else if(hasError(errMessage))
    {
      setStatus(IModelComponent::Failed ,  "Simulation failed with error message");
    }
    else
    {
      if(progressChecker()->performStep(m_currentDateTime->modifiedJulianDay()))
      {
        setStatus(IModelComponent::Updated , "Simulation performed time-step | DateTime: " + m_currentDateTime->dateTime().toString(Qt::ISODate) , progressChecker()->progress());
      }
      else
      {
        setStatus(IModelComponent::Updated);
      }
    }
  }
}

void SWMMComponent::finish()
{
  if(isPrepared())
  {
    setStatus(IModelComponent::Finishing , "SWMM simulation with component id " + id() + " is being disposed" , 100);

    disposeProject();

    setPrepared(false);
    setInitialized(false);

    setStatus(IModelComponent::Finished , "SWMM simulation with component id " + id() + " has been disposed" , 100);
    setStatus(IModelComponent::Created , "SWMM simulation with component id " + id() + " ran successfully and has been re-created" , 100);

  }
}

Project *SWMMComponent::project() const
{
  return m_SWMMProject;
}

void SWMMComponent::createDimensions()
{
  m_idDimension = new Dimension("IdDimension","Dimension for identifiers",this);
  m_timeDimension = new Dimension("TimeSeriesDimension","Dimension for times", this);
  m_geometryDimension = new Dimension("GeometryDimension","Dimension for geometries", this);
}

void SWMMComponent::createArguments()
{
  createInputFileArguments();
  createNodeAreasArguments();
  createNodePerimetersArguments();
  createNodeOrificeDischargeCoeffArguments();
}

void SWMMComponent::createInputFileArguments()
{
  QStringList fidentifiers;
  fidentifiers.append("Input File");
  fidentifiers.append("Output File");
  fidentifiers.append("Report File");

  Quantity* fquantity = Quantity::unitLessValues("InputFilesQuantity", QVariant::String, this);
  fquantity->setDefaultValue("");
  fquantity->setMissingValue("");

  m_inputFilesArgument = new IdBasedArgumentString("InputFiles", fidentifiers,m_idDimension,fquantity,this);
  m_inputFilesArgument->setCaption("Model Input Files");
  m_inputFilesArgument->addFileFilter("Input XML File (*.xml)");
  m_inputFilesArgument->setMatchIdentifiersWhenReading(true);

  addArgument(m_inputFilesArgument);
}

void SWMMComponent::createNodeAreasArguments()
{
  QStringList identifiers;
  Quantity* quantity = Quantity::areaInSquareMeters(this);
  quantity->setDefaultValue(1.75);
  quantity->setMissingValue(-99999999);

  m_nodeAreas = new IdBasedArgumentDouble("Node Opening Area",identifiers,m_idDimension,quantity,this);
  m_nodeAreas->setCaption("Node Opening Area");

  addArgument(m_nodeAreas);
}

void SWMMComponent::createNodePerimetersArguments()
{
  QStringList identifiers;
  Quantity* quantity = Quantity::lengthInMeters(this);
  quantity->setDefaultValue(2.75);
  quantity->setMissingValue(-99999999);

  m_nodePerimeters = new IdBasedArgumentDouble("Node Opening Perimeter",identifiers,m_idDimension,quantity,this);
  m_nodePerimeters->setCaption("Node Opening Perimeter");

  addArgument(m_nodePerimeters);
}

void SWMMComponent::createNodeOrificeDischargeCoeffArguments()
{
  QStringList identifiers;
  Quantity* quantity = Quantity::unitLessValues("Coefficient",QVariant::Double,this);
  quantity->setDefaultValue(0.6);
  quantity->setMissingValue(-99999999);

  m_nodeOrificeDischargeCoeffs = new IdBasedArgumentDouble("Node Orifice Discharge Coefficient",identifiers,m_idDimension,quantity,this);
  m_nodeOrificeDischargeCoeffs->setCaption("Node Orifice Discharge Coefficient");

  addArgument(m_nodeOrificeDischargeCoeffs);
}

bool SWMMComponent::initializeArguments(QString &message)
{
  if(mpiProcessRank() == 0)
  {
    return initializeInputFilesArguments(message) &&
        initializeNodeAreasArguments(message) &&
        initializeNodePerimetersArguments(message) &&
        initializeNodeOrificeDischargeCoeffArguments(message);
  }
  else
  {
    return true;
  }
}

bool SWMMComponent::initializeInputFilesArguments(QString &message)
{
  m_inputFiles.clear();

  QString inputFilePath = (*m_inputFilesArgument)["Input File"];
  QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

  if(inputFile.exists())
  {
    m_inputFiles["InputFile"]  = inputFile;
  }
  else
  {
    message = "Input file does not exist: " + inputFile.absoluteFilePath();
    return false;
  }


  QString outputFilePath = (*m_inputFilesArgument)["Output File"];
  QFileInfo outputFile = getAbsoluteFilePath(outputFilePath);

  if(outputFile.absoluteDir().exists())
  {
    m_inputFiles["OutputFile"] = outputFile;
  }
  else
  {
    message = "Output file directory does not exist: " + outputFile.absolutePath();
    return false;
  }

  QString reportFilePath = (*m_inputFilesArgument)["Report File"];
  QFileInfo reportFile = getAbsoluteFilePath(reportFilePath);

  if(reportFile.absoluteDir().exists())
  {
    m_inputFiles["ReportFile"] = reportFile;
  }
  else
  {
    message = "Report file directory does not exist: " + reportFile.absolutePath();
    return false;
  }

  char *inpF = new char[inputFile.absoluteFilePath().length() + 1] ;
  std::strcpy (inpF, inputFile.absoluteFilePath().toStdString().c_str());

  char *inpO = new char[outputFile.absoluteFilePath().length() + 1] ;
  std::strcpy (inpO, outputFile.absoluteFilePath().toStdString().c_str());

  char *inpR = new char[reportFile.absoluteFilePath().length() + 1] ;
  std::strcpy (inpR, reportFile.absoluteFilePath().toStdString().c_str());

  disposeProject();

  m_SWMMProject = swmm_createProject();

  if(swmm_open(m_SWMMProject, inpF,inpR,inpO) == 0)
  {
    int y, m, d, h, mm, s;
    datetime_decodeDate(m_SWMMProject->StartDateTime, &y,&m,&d);
    datetime_decodeTime(m_SWMMProject->StartDateTime, &h,&mm,&s);
    m_startDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));
    m_currentDateTime->setDateTime(m_startDateTime->dateTime());

    datetime_decodeDate(m_SWMMProject->EndDateTime, &y,&m,&d);
    datetime_decodeTime(m_SWMMProject->EndDateTime, &h,&mm,&s);
    m_endDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));

    currentDateTimeInternal()->setModifiedJulianDay(m_startDateTime->modifiedJulianDay());
    timeHorizonInternal()->setModifiedJulianDay(m_startDateTime->modifiedJulianDay());
    timeHorizonInternal()->setDuration(m_endDateTime->modifiedJulianDay() - m_startDateTime->modifiedJulianDay());

    swmm_start(m_SWMMProject, TRUE);
    progressChecker()->reset(m_startDateTime->modifiedJulianDay(), m_endDateTime->modifiedJulianDay());

    delete[] inpF;
    delete[] inpO;
    delete[] inpR;


    QFile file(inputFile.absoluteFilePath());

    if(file.open(QIODevice::ReadOnly))
    {

      QRegExp delimiter("(\\,|\\t|\\;|\\ )");
      QTextStream streamReader(&file);

      while (!streamReader.atEnd())
      {
        QString line = streamReader.readLine().trimmed();

        if(!line.compare("[POLYGONS]",Qt::CaseInsensitive))
        {
          initializeSubCatchmentGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[COORDINATES]",Qt::CaseInsensitive))
        {
          initializeNodeGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[VERTICES]",Qt::CaseInsensitive))
        {
          initializeLinkGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[SYMBOLS]",Qt::CaseInsensitive))
        {
        }
      }
    }
  }

  return !hasError(message);

}

bool SWMMComponent::initializeNodeAreasArguments(QString &message)
{
  double defaultArea = m_nodeAreas->valueDefinition()->defaultValue().toDouble();

  for(int i = 0; i < m_SWMMProject->NumNodes; i++)
  {
    TNode &node = m_SWMMProject->Node[i];
    QString id = QString(node.ID);

    if(m_nodeAreas->containsIdentifier(id))
    {
      node.area = (*m_nodeAreas)[id];
    }
    else
    {
      node.area = defaultArea;
    }
  }

  message = "";

  return true;
}

bool SWMMComponent::initializeNodePerimetersArguments(QString &message)
{
  double defaultPerimeter = m_nodePerimeters->valueDefinition()->defaultValue().toDouble();

  for(int i = 0; i < m_SWMMProject->NumNodes; i++)
  {
    TNode &node = m_SWMMProject->Node[i];
    QString id = QString(node.ID);

    if(m_nodePerimeters->containsIdentifier(id))
    {
      node.perimeter = (*m_nodePerimeters)[id];
    }
    else
    {
      node.perimeter = defaultPerimeter;
    }
  }

  message = "";

  return true;
}

bool SWMMComponent::initializeNodeOrificeDischargeCoeffArguments(QString &message)
{
  double defaultCoeff = m_nodeOrificeDischargeCoeffs->valueDefinition()->defaultValue().toDouble();

  for(int i = 0; i < m_SWMMProject->NumNodes; i++)
  {
    TNode &node = m_SWMMProject->Node[i];
    QString id = QString(node.ID);

    if(m_nodeOrificeDischargeCoeffs->containsIdentifier(id))
    {
      node.orificeDischargeCoeff = (*m_nodeOrificeDischargeCoeffs)[id];
    }
    else
    {
      node.orificeDischargeCoeff = defaultCoeff;
    }
  }

  message = "";

  return true;
}

void SWMMComponent::initializeNodeGeometries(QTextStream &streamReader, const QRegExp &delimiter)
{
  double x =  0, y = 0;
  m_sharedNodesGeoms.clear();
  //  QList<HCGeometry*> nodes;

  for(int i = 0; i < m_SWMMProject->NumNodes; i++)
  {
    TNode &node = m_SWMMProject->Node[i];
    QString id = QString(node.ID);
    QSharedPointer<HCGeometry> pt = QSharedPointer<HCGeometry>(new HCPoint(id));
    pt->setIndex(i);
    m_sharedNodesGeoms[id] = pt;
  }

  while (!streamReader.atEnd())
  {
    QString line = streamReader.readLine();

    if(line.isEmpty() || line.isNull())
    {
      break;
    }
    else if(line.trimmed()[0] != ';')
    {
      QStringList splitted = line.split(delimiter,QString::SkipEmptyParts);

      if(splitted.length() > 2)
      {
        bool xok = false, yok = false;
        x = splitted[1].toDouble(&xok);
        y = splitted[2].toDouble(&yok);

        if(xok && yok)
        {
          QString id = splitted[0].trimmed();

          if(m_sharedNodesGeoms.find(id) != m_sharedNodesGeoms.end())
          {
            QSharedPointer<HCGeometry> geom = m_sharedNodesGeoms[id];
            HCPoint *point  = dynamic_cast<HCPoint*>(geom.data());
            point->setX(x);
            point->setY(y);
            //            nodes.append(point);
          }
        }
      }
    }
  }

  //  QString message;
  //  GeometryFactory::writeGeometryToFile(nodes,"Nodes",nodes[0]->geometryType(), "ESRI Shapefile","/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/SWMM_Model/GIS/nodes.shp",message);

}

void SWMMComponent::initializeLinkGeometries(QTextStream &streamReader, const QRegExp &delimiter)
{
  double x =  0, y = 0;
  m_sharedLinkGeoms.clear();

  for(int i = 0; i < m_SWMMProject->NumLinks; i++)
  {
    TLink &link = m_SWMMProject->Link[i];
    QString id = QString(link.ID);
    HCLineString *lineString =  new HCLineString(id);
    lineString->setIndex(i);

    TNode &node1 = m_SWMMProject->Node[link.node1];
    QString nodeId = QString(node1.ID);
    HCPoint *node1Geom = dynamic_cast<HCPoint*>(m_sharedNodesGeoms[nodeId].data());

    HCPoint *beginP = new HCPoint(node1Geom->x(), node1Geom->y(), node1Geom->id());
    lineString->addPoint(beginP);

    m_sharedLinkGeoms[id] = QSharedPointer<HCGeometry>(lineString);
  }

  while (!streamReader.atEnd())
  {
    QString line = streamReader.readLine();

    if(line.isEmpty() || line.isNull())
    {
      break;
    }
    else if(line.trimmed()[0] != ';')
    {
      QStringList splitted = line.split(delimiter,QString::SkipEmptyParts);

      if(splitted.length() > 2)
      {
        bool xok = false, yok = false;
        x = splitted[1].toDouble(&xok);
        y = splitted[2].toDouble(&yok);

        if(xok && yok)
        {
          QString id = splitted[0].trimmed();

          if(m_sharedLinkGeoms.find(id) != m_sharedLinkGeoms.end())
          {
            QSharedPointer<HCGeometry> geom = m_sharedLinkGeoms[id];
            HCLineString *lineString  = dynamic_cast<HCLineString*>(geom.data());
            HCPoint *point = new HCPoint();
            point->setX(x);
            point->setY(y);
            lineString->addPoint(point);
          }
        }
      }
    }
  }

  //  QList<HCGeometry*> links;

  for(int i = 0; i < m_SWMMProject->NumLinks; i++)
  {
    TLink &link = m_SWMMProject->Link[i];
    QString id = QString(link.ID);
    HCLineString *lineString = dynamic_cast<HCLineString*>(m_sharedLinkGeoms[id].data());

    TNode &node2 = m_SWMMProject->Node[link.node2];
    QString nodeId = QString(node2.ID);
    HCPoint *node2Geom = dynamic_cast<HCPoint*>(m_sharedNodesGeoms[nodeId].data());

    HCPoint *endP = new HCPoint(node2Geom->x(), node2Geom->y(), node2Geom->id());
    lineString->addPoint(endP);
    //    links.append(lineString);
  }

  //  QString message;
  //  GeometryFactory::writeGeometryToFile(links,"Links",links[0]->geometryType(), "ESRI Shapefile","/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/SWMM_Model/GIS/conduits.shp", message);

}

void SWMMComponent::initializeSubCatchmentGeometries(QTextStream &streamReader, const QRegExp &delimiter)
{

}

void SWMMComponent::createInputs()
{
  initializeIdBasedNodeWSEInput();
  initializeIdBasedNodeInflowInput();
  initializeNodeWSEInput();
  initializeNodePondedDepthInput();
  initializeNodeInflowInput();
  initializePondedAreaInput();
}

void SWMMComponent::initializeIdBasedNodeWSEInput()
{

}

void SWMMComponent::initializeIdBasedNodeInflowInput()
{

}

void SWMMComponent::initializeNodeWSEInput()
{
  Quantity *elevQuantity = Quantity::lengthInMeters(this);
  elevQuantity->setCaption("Water Surface Elevation (m)");

  m_nodeWSEInput = new NodeWSEInput("NodeWSEInput",
                                    m_timeDimension,
                                    m_geometryDimension,
                                    elevQuantity,
                                    this);

  m_nodeWSEInput->setCaption("Node Water Surface Elevation (m)");

  QList<QSharedPointer<HCGeometry>> geometries;

  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedNodesGeoms)
  {
    geometries.append(node.second);
  }

  m_nodeWSEInput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay() - 1.0/10000000000.0, m_nodeWSEInput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay(), m_nodeWSEInput);
  m_nodeWSEInput->addTime(dt1);
  m_nodeWSEInput->addTime(dt2);

  addInput(m_nodeWSEInput);
}

void SWMMComponent::initializeNodePondedDepthInput()
{
  Quantity *depthQuantity = Quantity::lengthInMeters(this);
  depthQuantity->setCaption("Depth (m)");

  m_nodePondedDepth = new NodePondedDepthInput("NodePondedDepthInput",
                                               m_timeDimension,
                                               m_geometryDimension,
                                               depthQuantity,
                                               this);

  m_nodePondedDepth->setCaption("Node Ponded Depth (m)");

  QList<QSharedPointer<HCGeometry>> geometries;

  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedNodesGeoms)
  {
    geometries.append(node.second);
  }

  m_nodePondedDepth->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay()- 1.0/10000000000.0, m_nodePondedDepth);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay(), m_nodePondedDepth);
  m_nodePondedDepth->addTime(dt1);
  m_nodePondedDepth->addTime(dt2);

  addInput(m_nodePondedDepth);
}

void SWMMComponent::initializeNodeInflowInput()
{

}

void SWMMComponent::initializePondedAreaInput()
{
  Quantity *areaValueDefinition = Quantity::areaInSquareMeters(this);

  m_pondedAreaInput = new PondedAreaInput("Ponded Area", m_geometryDimension, areaValueDefinition,this);
  m_pondedAreaInput->setCaption("Node Ponded Areas (m^2)");

  QList<QSharedPointer<HCGeometry>> geometries;

  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedNodesGeoms)
  {
    geometries.append(node.second);
  }

  m_pondedAreaInput->addGeometries(geometries);

  addInput(m_pondedAreaInput);

}

void SWMMComponent::intializeSurfaceInflows()
{
  m_surfaceInflow.clear();

  for(int i = 0; i < m_SWMMProject->NumNodes ; i++)
  {
    m_surfaceInflow[i] = 0.0;
  }
}

void SWMMComponent::createOutputs()
{
  initializeIdBasedNodeWSEOutput();
  initializeIdBasedLinkFlowOutput();
  initializeNodeWSEOutput();
  initializeNodeFloodingOutput();
  initializeLinkFlowOutput();
}

void SWMMComponent::initializeIdBasedNodeWSEOutput()
{

}

void SWMMComponent::initializeIdBasedLinkFlowOutput()
{

}

void SWMMComponent::initializeNodeWSEOutput()
{

}

void SWMMComponent::initializeNodeFloodingOutput()
{
  Quantity *flowQuantity = Quantity::flowInCMS(this);

  m_nodeSurfaceFlowOutput = new NodeSurfaceFlowOutput("NodeSurfaceFlow",
                                                      m_timeDimension,
                                                      m_geometryDimension,
                                                      flowQuantity,
                                                      this);

  m_nodeSurfaceFlowOutput->setCaption("Inflow/Outflows from Nodes (m^3/s)");
  m_nodeSurfaceFlowOutput->setDescription("May be positive or negative");

  QList<QSharedPointer<HCGeometry>> geometries;
  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedNodesGeoms)
  {
    geometries.append(node.second);
  }

  m_nodeSurfaceFlowOutput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay() - 1.0/10000000000.0, m_nodeSurfaceFlowOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay(), m_nodeSurfaceFlowOutput);
  m_nodeSurfaceFlowOutput->addTime(dt1);
  m_nodeSurfaceFlowOutput->addTime(dt2);

  addOutput(m_nodeSurfaceFlowOutput);
}

void SWMMComponent::initializeLinkFlowOutput()
{
  Quantity *flowQuantity = Quantity::flowInCMS(this);

  m_linkFlowOutput = new LinkFlowOutput("LinkFlowOutput",
                                        m_timeDimension,
                                        m_geometryDimension,
                                        flowQuantity,
                                        this);

  m_linkFlowOutput->setCaption("Flow through conduit (m^3/s)");
  m_linkFlowOutput->setDescription("May be positive or negative");

  QList<QSharedPointer<HCGeometry>> geometries;
  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedLinkGeoms)
  {
    geometries.append(node.second);
  }

  m_linkFlowOutput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay() - 1.0/10000000000.0, m_linkFlowOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(m_startDateTime->modifiedJulianDay(), m_linkFlowOutput);
  m_linkFlowOutput->addTime(dt1);
  m_linkFlowOutput->addTime(dt2);

  addOutput(m_linkFlowOutput);
}

bool SWMMComponent::hasError(QString &message)
{
  message = "";

  int error = m_SWMMProject->ErrorCode;

  //  if(error)
  //  {
  //    message = QString(getErrorMsg(m_SWMMProject->ErrorCode)).trimmed();
  //    return true;
  //  }

  return false;

}

void SWMMComponent::intializeFailureCleanUp()
{
  disposeProject();
}

void SWMMComponent::disposeProject()
{
  if(m_SWMMProject)
  {
    swmm_end(m_SWMMProject);

    if (m_SWMMProject->Fout.mode == SCRATCH_FILE)
    {
      swmm_report(m_SWMMProject);
    }

    swmm_close(m_SWMMProject);

    m_SWMMProject = nullptr;
  }
}

void SWMMComponent::resetSurfaceInflows()
{

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
  for(size_t i = 0; i < m_surfaceInflow.size() ; i++)
  {
    m_surfaceInflow[i] = 0.0;
  }
}

