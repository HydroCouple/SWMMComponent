#include "stdafx.h"

#include <QDebug>
#include <QDir>
#include <cstring>
#include <QString>
#include <QUuid>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "core/idbasedinputs.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/idbasedinputs.h"
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
#include "linkdepthoutput.h"
#include "conduitxsectareaoutput.h"
#include "conduittopwidthoutput.h"
#include "conduitbankxsectareaoutput.h"
#include "spatial/geometryfactory.h"


using namespace std;
using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

SWMMComponent::SWMMComponent(const QString &id, SWMMComponentInfo *parent)
  :AbstractTimeModelComponent(id,parent),
    m_SWMMComponentInfo(parent),
    m_inputFilesArgument(nullptr),
    m_nodeAreas(nullptr),
    m_nodePerimeters(nullptr),
    m_nodeOrificeDischargeCoeffs(nullptr),
    m_SWMMProject(nullptr),
    m_nodeSurfaceFlowOutput(nullptr),
    m_linkFlowOutput(nullptr),
    m_linkDepthOutput(nullptr),
    m_conduitXSectAreaOutput(nullptr),
    m_conduitTopWidthOutput(nullptr),
    m_conduitBankXSectAreaOutput(nullptr),
    m_pondedAreaInput(nullptr),
    m_nodeWSEInput(nullptr),
    m_nodePondedDepth(nullptr),
    m_idDimension(nullptr),
    m_geometryDimension(nullptr),
    m_timeDimension(nullptr),
    m_currentProgress(0)
{

  createDimensions();
  createArguments();
}

SWMMComponent::SWMMComponent(const QString &id, const QString &caption, SWMMComponentInfo *parent)
  :AbstractTimeModelComponent(id, caption, parent),
    m_SWMMComponentInfo(parent),
    m_inputFilesArgument(nullptr),
    m_nodeAreas(nullptr),
    m_nodePerimeters(nullptr),
    m_nodeOrificeDischargeCoeffs(nullptr),
    m_SWMMProject(nullptr),
    m_nodeSurfaceFlowOutput(nullptr),
    m_linkFlowOutput(nullptr),
    m_linkDepthOutput(nullptr),
    m_conduitXSectAreaOutput(nullptr),
    m_conduitTopWidthOutput(nullptr),
    m_conduitBankXSectAreaOutput(nullptr),
    m_pondedAreaInput(nullptr),
    m_nodeWSEInput(nullptr),
    m_nodePondedDepth(nullptr),
    m_idDimension(nullptr),
    m_geometryDimension(nullptr),
    m_timeDimension(nullptr),
    m_currentProgress(0),
    m_parent(nullptr)
{
  createDimensions();
  createArguments();
}

SWMMComponent::~SWMMComponent()
{
  disposeProject();

  while (m_clones.size())
  {
    SWMMComponent *clone = dynamic_cast<SWMMComponent*>(m_clones.first());
    removeClone(clone);
    delete clone;
  }

  if(m_parent)
  {
    m_parent->removeClone(this);
    m_parent = nullptr;
  }


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

    //    QString inputFilePath = (*m_inputFilesArgument)["Input File"];
    //    QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

    //    if(inputFile.exists())
    //    {

    //      if(m_scratchFileIO.isOpen())
    //        m_scratchFileIO.close();

    //      QString scratchFile = inputFile.absoluteFilePath().replace(inputFile.suffix(), "scratch");
    //      m_scratchFileIO.setFileName(scratchFile);

    //      if(m_scratchFileIO.open(QIODevice::WriteOnly | QIODevice::Truncate))
    //      {
    //        m_scratchFileTextStream.setDevice(&m_scratchFileIO);
    //        m_scratchFileTextStream.setRealNumberPrecision(10);
    //        m_scratchFileTextStream << "IDStartPos" << "\t" << m_SWMMProject->IDStartPos << endl;
    //        m_scratchFileTextStream << "InputStartPos" << "\t" << m_SWMMProject->InputStartPos << endl;
    //        m_scratchFileTextStream << "OutputStartPos" << "\t" << m_SWMMProject->OutputStartPos << endl;
    //        m_scratchFileTextStream.flush();
    //      }
    //    }

    createSurfaceInflows();
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
    setStatus(IModelComponent::Updating);

    double minConsumerTime = std::max(currentDateTime()->julianDay(), getMinimumConsumerTime());

    double elapsedTime = 0;

    while(currentDateTime()->julianDay() <= minConsumerTime)
    {
      clearDataCache(m_SWMMProject);
      resetSurfaceInflows();
      applyInputValues();

      swmm_step(m_SWMMProject, &elapsedTime);

      m_timeStep = (elapsedTime - (currentDateTime()->julianDay() - timeHorizon()->julianDay())) * 86400.0;

      double newDateTime = timeHorizon()->julianDay() + elapsedTime;
      currentDateTimeInternal()->setJulianDay(newDateTime);

      if(elapsedTime == 0.0)
      {
        break;
      }
      else if(progressChecker()->performStep(currentDateTime()->julianDay()))
      {
        setStatus(IModelComponent::Updated , "Simulation performed time-step | DateTime: " + QString::number(currentDateTime()->julianDay(), 'f') , progressChecker()->progress());
      }
    }

    updateOutputValues(requiredOutputs);

    QString errMessage;

    if(elapsedTime <= 0.0 && !m_SWMMProject->ErrorCode)
    {
      setStatus(IModelComponent::Done , "Simulation finished successfully|  DateTime: " + QString::number(currentDateTime()->julianDay(), 'f'),100);
    }
    else if(hasError(errMessage))
    {
      setStatus(IModelComponent::Failed ,  "Simulation failed with error message");
    }
    else
    {
      if(progressChecker()->performStep(currentDateTime()->julianDay()))
      {
        setStatus(IModelComponent::Updated , "Simulation performed time-step | DateTime: " + QString::number(currentDateTime()->julianDay(), 'f') , progressChecker()->progress());
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

HydroCouple::ICloneableModelComponent* SWMMComponent::parent() const
{
  return m_parent;
}

HydroCouple::ICloneableModelComponent* SWMMComponent::clone()
{
  if(isInitialized())
  {
    SWMMComponent *cloneComponent = dynamic_cast<SWMMComponent*>(m_SWMMComponentInfo->createComponentInstance());
    cloneComponent->setReferenceDirectory(referenceDirectory());

    IdBasedArgumentString *identifierArg = identifierArgument();
    IdBasedArgumentString *cloneIndentifierArg = cloneComponent->identifierArgument();

    (*cloneIndentifierArg)["Id"] = QString((*identifierArg)["Id"]);
    (*cloneIndentifierArg)["Caption"] = QString((*identifierArg)["Caption"]);
    (*cloneIndentifierArg)["Description"] = QString((*identifierArg)["Description"]);

    QString appendName = "_clone_" + QString::number(m_clones.size()) + "_" + QUuid::createUuid().toString().replace("{","").replace("}","");

    //(*cloneComponent->m_inputFilesArgument)["Input File"] = QString((*m_inputFilesArgument)["Input File"]);

    QString inputFilePath = QString((*m_inputFilesArgument)["Input File"]);
    QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

    if(inputFile.absoluteDir().exists())
    {
      QString suffix = "." + inputFile.completeSuffix();
      inputFilePath = inputFile.absoluteFilePath().replace(suffix,"") + appendName + suffix;
      QFile::copy(inputFile.absoluteFilePath(), inputFilePath);
      (*cloneComponent->m_inputFilesArgument)["Input File"] = inputFilePath;
    }

    QString outputFilePath = QString((*m_inputFilesArgument)["Output File"]);
    QFileInfo outputFile = getAbsoluteFilePath(outputFilePath);

    if(outputFile.absoluteDir().exists())
    {
      QString suffix = "." + outputFile.completeSuffix();
      outputFilePath = outputFile.absoluteFilePath().replace(suffix,"") + appendName + suffix;
      (*cloneComponent->m_inputFilesArgument)["Output File"] = outputFilePath;
    }

    QString reportFilePath = QString((*m_inputFilesArgument)["Report File"]);
    QFileInfo reportFile = getAbsoluteFilePath(reportFilePath);

    if(!reportFilePath.isEmpty() && reportFile.absoluteDir().exists())
    {
      QString suffix = "." + reportFile.completeSuffix();
      reportFilePath = reportFile.absoluteFilePath().replace(suffix,"") + appendName + suffix;
      (*cloneComponent->m_inputFilesArgument)["Report File"] = reportFilePath;
    }

    cloneComponent->m_parent = this;
    m_clones.append(cloneComponent);

    emit propertyChanged("Clones");

#ifdef USE_OPENMP
#pragma omp critical (SWMMComponent)
#endif
    {
      cloneComponent->initialize();
    }

    return cloneComponent;
  }

  return nullptr;
}

QList<HydroCouple::ICloneableModelComponent*> SWMMComponent::clones() const
{
  return m_clones;
}

Project *SWMMComponent::project() const
{
  return m_SWMMProject;
}

bool SWMMComponent::removeClone(SWMMComponent *component)
{
  int removed;

#ifdef USE_OPENMP
#pragma omp critical (SWMMComponent)
#endif
  {
    removed = m_clones.removeAll(component);
  }


  if(removed)
  {
    component->m_parent = nullptr;
    emit propertyChanged("Clones");
  }

  return removed;
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

  if(!reportFilePath.isEmpty() && reportFile.absoluteDir().exists())
  {
    m_inputFiles["ReportFile"] = reportFile;
  }
  else
  {
    message = "Report file directory does not exist: " + reportFile.absolutePath();
    return false;
  }

  disposeProject();

  char *m_inputFile = nullptr;
  char *m_reportFile = nullptr;
  char *m_outputFile = nullptr;

  m_inputFile = new char[inputFile.absoluteFilePath().length() + 1] ;
  std::strcpy (m_inputFile, inputFile.absoluteFilePath().toStdString().c_str());

  if(outputFile.dir().exists())
  {
    m_outputFile = new char[outputFile.absoluteFilePath().length() + 1] ;
    std::strcpy (m_outputFile, outputFile.absoluteFilePath().toStdString().c_str());
  }

  if(reportFile.dir().exists())
  {
    m_reportFile = new char[reportFile.absoluteFilePath().length() + 1] ;
    std::strcpy (m_reportFile, reportFile.absoluteFilePath().toStdString().c_str());
  }

  swmm_createProject(&m_SWMMProject);

  QString current = QDir::currentPath();
  QDir::setCurrent(inputFile.absolutePath());

  if(swmm_open(m_SWMMProject, m_inputFile, m_reportFile, m_outputFile) == 0 && swmm_start(m_SWMMProject, TRUE) == 0)
  {
    int y, m, d, h, mm, s;
    datetime_decodeDate(m_SWMMProject->StartDateTime, &y,&m,&d);
    datetime_decodeTime(m_SWMMProject->StartDateTime, &h,&mm,&s);

    QDateTime dateTime(QDate(y,m,d),QTime(h,mm,s));

    SDKTemporal::TimeSpan *timeSpan = timeHorizonInternal();
    SDKTemporal::DateTime *currentDateTime = currentDateTimeInternal();

    currentDateTime->setDateTime(dateTime);
    timeSpan->setJulianDay(currentDateTime->julianDay());
    timeSpan->setDuration(m_SWMMProject->EndDateTime - m_SWMMProject->StartDateTime);

    progressChecker()->reset(timeSpan->julianDay(), timeSpan->endDateTime());

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
          readSubCatchmentGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[COORDINATES]",Qt::CaseInsensitive))
        {
          readNodeGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[VERTICES]",Qt::CaseInsensitive))
        {
          readLinkGeometries(streamReader, delimiter);
        }
        else if(!line.compare("[SYMBOLS]",Qt::CaseInsensitive))
        {
        }
      }
    }
  }

  if(m_inputFile)
    delete[] m_inputFile;

  if(m_reportFile)
    delete[] m_reportFile;

  if(m_outputFile)
    delete[] m_outputFile;

  QDir::setCurrent(current);

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

void SWMMComponent::readNodeGeometries(QTextStream &streamReader, const QRegExp &delimiter)
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
    pt->setMarker(i);
    m_sharedNodesGeoms[id] = pt;
  }

  while (!streamReader.atEnd())
  {
    QString line = streamReader.readLine().trimmed();

    if(line.isEmpty() || line.isNull())
    {
      break;
    }
    else if(line[0] != ';')
    {
      QStringList splitted = line.split(delimiter,QString::SkipEmptyParts);

      if(splitted.length() > 2)
      {
        bool xok = false, yok = false;
        x = splitted[1].toDouble(&xok);
        y = splitted[2].toDouble(&yok);

        if(xok && yok)
        {
          QString id = splitted[0];

          if(m_sharedNodesGeoms.find(id) != m_sharedNodesGeoms.end())
          {
            QSharedPointer<HCGeometry> geom = m_sharedNodesGeoms[id];
            HCPoint *point  = dynamic_cast<HCPoint*>(geom.data());
            point->setX(x);
            point->setY(y);
          }
        }
      }
    }
  }
}

void SWMMComponent::readLinkGeometries(QTextStream &streamReader, const QRegExp &delimiter)
{
  double x =  0, y = 0;
  m_sharedLinkGeoms.clear();

  for(int i = 0; i < m_SWMMProject->NumLinks; i++)
  {
    TLink &link = m_SWMMProject->Link[i];
    QString id = QString(link.ID);
    HCLineString *lineString =  new HCLineString(id);
    lineString->setIndex(i);
    lineString->setMarker(i);

    TNode &node1 = m_SWMMProject->Node[link.node1];
    QString nodeId = QString(node1.ID);
    HCPoint *node1Geom = dynamic_cast<HCPoint*>(m_sharedNodesGeoms[nodeId].data());

    HCPoint *beginP = new HCPoint(node1Geom->x(), node1Geom->y(), node1Geom->id() , lineString);
    lineString->addPoint(beginP);

    m_sharedLinkGeoms[id] = QSharedPointer<HCGeometry>(lineString);
  }

  while (!streamReader.atEnd())
  {
    QString line = streamReader.readLine().trimmed();

    if(line.isEmpty() || line.isNull())
    {
      break;
    }
    else if(line[0] != ';')
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
            HCPoint *point = new HCPoint(id, lineString);
            point->setX(x);
            point->setY(y);
            lineString->addPoint(point);
          }
        }
      }
    }
  }

  for(int i = 0; i < m_SWMMProject->NumLinks; i++)
  {
    TLink &link = m_SWMMProject->Link[i];
    QString id = QString(link.ID);

    HCLineString *lineString = dynamic_cast<HCLineString*>(m_sharedLinkGeoms[id].data());

    TNode &node2 = m_SWMMProject->Node[link.node2];
    QString nodeId = QString(node2.ID);
    HCPoint *node2Geom = dynamic_cast<HCPoint*>(m_sharedNodesGeoms[nodeId].data());

    HCPoint *endP = new HCPoint(node2Geom->x(), node2Geom->y(), node2Geom->id(), lineString);
    lineString->addPoint(endP);
  }
}

void SWMMComponent::readSubCatchmentGeometries(QTextStream &streamReader, const QRegExp &delimiter)
{

}

void SWMMComponent::createInputs()
{
  createIdBasedNodeWSEInput();
  createIdBasedNodeInflowInput();
  createNodeWSEInput();
  createNodePondedDepthInput();
  createNodeInflowInput();
  createPondedAreaInput();
}

void SWMMComponent::createIdBasedNodeWSEInput()
{

}

void SWMMComponent::createIdBasedNodeInflowInput()
{

}

void SWMMComponent::createNodeWSEInput()
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

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_nodeWSEInput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_nodeWSEInput);
  m_nodeWSEInput->addTime(dt1);
  m_nodeWSEInput->addTime(dt2);

  addInput(m_nodeWSEInput);
}

void SWMMComponent::createNodePondedDepthInput()
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

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay()- 1.0/1000000.0, m_nodePondedDepth);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_nodePondedDepth);
  
  m_nodePondedDepth->addTime(dt1);
  m_nodePondedDepth->addTime(dt2);

  addInput(m_nodePondedDepth);
}

void SWMMComponent::createNodeInflowInput()
{

}

void SWMMComponent::createPondedAreaInput()
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

void SWMMComponent::createSurfaceInflows()
{
  m_surfaceInflow.clear();

  for(int i = 0; i < m_SWMMProject->NumNodes ; i++)
  {
    m_surfaceInflow[i] = 0.0;
  }
}

void SWMMComponent::createOutputs()
{
  createIdBasedNodeWSEOutput();
  createIdBasedLinkFlowOutput();
  createNodeWSEOutput();
  createNodeFloodingOutput();
  createLinkFlowOutput();
  createLinkDepthOutput();
  createConduitXSectAreaOutput();
  createConduitTopWidthOutput();
  createConduitBankXSectAreaOutput();
}

void SWMMComponent::createIdBasedNodeWSEOutput()
{

}

void SWMMComponent::createIdBasedLinkFlowOutput()
{

}

void SWMMComponent::createNodeWSEOutput()
{

}

void SWMMComponent::createNodeFloodingOutput()
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

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_nodeSurfaceFlowOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_nodeSurfaceFlowOutput);
  m_nodeSurfaceFlowOutput->addTime(dt1);
  m_nodeSurfaceFlowOutput->addTime(dt2);

  addOutput(m_nodeSurfaceFlowOutput);
}

void SWMMComponent::createLinkFlowOutput()
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

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_linkFlowOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_linkFlowOutput);
  m_linkFlowOutput->addTime(dt1);
  m_linkFlowOutput->addTime(dt2);

  addOutput(m_linkFlowOutput);
}

void SWMMComponent::createLinkDepthOutput()
{
  Quantity *depthQuantity = Quantity::lengthInMeters(this);

  m_linkDepthOutput = new LinkDepthOutput("LinkDepthOutput",
                                          m_timeDimension,
                                          m_geometryDimension,
                                          depthQuantity,
                                          this);

  m_linkDepthOutput->setCaption("Depth of water in conduit (m)");
  m_linkDepthOutput->setDescription("Depth of water in conduit (m)");

  QList<QSharedPointer<HCGeometry>> geometries;
  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedLinkGeoms)
  {
    geometries.append(node.second);
  }

  m_linkDepthOutput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_linkDepthOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_linkDepthOutput);
  m_linkDepthOutput->addTime(dt1);
  m_linkDepthOutput->addTime(dt2);

  addOutput(m_linkDepthOutput);
}

void SWMMComponent::createConduitXSectAreaOutput()
{
  Quantity *areaQuantity = Quantity::areaInSquareMeters(this);

  m_conduitXSectAreaOutput = new ConduitXSectAreaOutput("ConduitCrossSectionAreaOutput",
                                                        m_timeDimension,
                                                        m_geometryDimension,
                                                        areaQuantity,
                                                        this);

  m_conduitXSectAreaOutput->setCaption("Conduit flow cross section area (m^2)");
  m_conduitXSectAreaOutput->setDescription("Conduit flow cross section area (m^2)");

  QList<QSharedPointer<HCGeometry>> geometries;
  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedLinkGeoms)
  {
    geometries.append(node.second);
  }

  m_conduitXSectAreaOutput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_conduitXSectAreaOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_conduitXSectAreaOutput);
  m_conduitXSectAreaOutput->addTime(dt1);
  m_conduitXSectAreaOutput->addTime(dt2);

  addOutput(m_conduitXSectAreaOutput);
}

void SWMMComponent::createConduitTopWidthOutput()
{
  Quantity *lengthQuantity = Quantity::lengthInMeters(this);

  m_conduitTopWidthOutput = new ConduitTopWidthOutput("ConduitTopWidthOutput",
                                                      m_timeDimension,
                                                      m_geometryDimension,
                                                      lengthQuantity,
                                                      this);

  m_conduitTopWidthOutput->setCaption("Conduit top width (m)");
  m_conduitTopWidthOutput->setDescription("Conduit top width (m)");

  QList<QSharedPointer<HCGeometry>> geometries;
  for(std::pair<QString, QSharedPointer<HCGeometry>> node : m_sharedLinkGeoms)
  {
    geometries.append(node.second);
  }

  m_conduitTopWidthOutput->addGeometries(geometries);

  SDKTemporal::DateTime *dt1 = new SDKTemporal::DateTime(timeHorizon()->julianDay() - 1.0/1000000.0, m_conduitTopWidthOutput);
  SDKTemporal::DateTime *dt2 = new SDKTemporal::DateTime(timeHorizon()->julianDay(), m_conduitTopWidthOutput);
  m_conduitTopWidthOutput->addTime(dt1);
  m_conduitTopWidthOutput->addTime(dt2);

  addOutput(m_conduitTopWidthOutput);
}

void SWMMComponent::createConduitBankXSectAreaOutput()
{

}

bool SWMMComponent::hasError(QString &message)
{
  message = "";

  if(m_SWMMProject->ErrorCode)
  {
    message = QString(m_SWMMProject->ErrorMsg).trimmed();
    return true;
  }

  return false;

}

void SWMMComponent::initializeFailureCleanUp()
{
  disposeProject();
}

void SWMMComponent::disposeProject()
{
  if(m_SWMMProject)
  {
    clearDataCache(m_SWMMProject);

    swmm_end(m_SWMMProject);

    if(m_SWMMProject->Fout.mode == SCRATCH_FILE)
      swmm_report(m_SWMMProject);

    swmm_close(m_SWMMProject);
    swmm_deleteProject(m_SWMMProject);

    m_SWMMProject = nullptr;
  }
}

void SWMMComponent::resetSurfaceInflows()
{
  //#ifdef USE_OPENMP
  //#pragma omp parallel for
  //#endif
  for(size_t i = 0; i < m_surfaceInflow.size() ; i++)
  {
    m_surfaceInflow[i] = 0.0;
  }
}

