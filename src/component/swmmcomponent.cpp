#include "stdafx.h"
#include "core/idbasedcomponentdataitem.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/idbasedcomponentdataitem.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/unit.h"
#include "core/swmm5.h"
#include "core/funcs.h"
#include "component/swmmcomponent.h"
#include "component/swmmmodelcomponentinfo.h"
#include "component/swmmtimeseriesexchangeitems.h"
#include "temporal/temporalinterpolationfactory.h"

#include <QDebug>
#include <QDir>

using namespace SDKTemporal;

SWMMComponent::SWMMComponent(const QString &id, SWMMComponent *component)
  :AbstractModelComponent(id,component),
    m_identifiersArgument(nullptr),
    m_inputFilesArgument(nullptr),
    m_usedNodesArgument(nullptr),
    m_usedLinksArgument(nullptr),
    m_usedSubCatchmentsArgument(nullptr),
    m_SWMMProject(nullptr),
    m_initialized(false),
    m_prepared(false),
    m_currentProgress(0)
{
  m_startDateTime = new Time(this);
  m_endDateTime = new Time(this);
  m_currentDateTime = new Time(this);
  createArguments();
  createAdaptedOutputFactories();
}

SWMMComponent::SWMMComponent(const QString &id, const QString &caption, SWMMComponent *component)
  :AbstractModelComponent(id, caption, component),
    m_identifiersArgument(nullptr),
    m_inputFilesArgument(nullptr),
    m_usedNodesArgument(nullptr),
    m_usedLinksArgument(nullptr),
    m_usedSubCatchmentsArgument(nullptr),
    m_SWMMProject(nullptr),
    m_currentProgress(0)
{
  m_startDateTime = new Time(this);
  m_endDateTime = new Time(this);
  m_currentDateTime = new Time(this);
  createArguments();
  createAdaptedOutputFactories();
}

SWMMComponent::~SWMMComponent()
{
  if(m_SWMMProject)
  {
    swmm_end(m_SWMMProject);
    swmm_close(m_SWMMProject);
  }
}

void SWMMComponent::createArguments()
{

  //Identifiers
  {
    Dimension *identifierDimension = new Dimension("IdentifierDimension","Dimension for identifiers", this);
    QStringList identifiers;
    identifiers.append("Id");
    identifiers.append("Caption");
    identifiers.append("Description");
    Quantity* quantity = Quantity::unitLessValues("IdentifiersUnit","", QVariant::String , this);

    m_identifiersArgument = new IdBasedArgumentQString("Identifiers", identifiers,identifierDimension,quantity,this);
    m_identifiersArgument->setCaption("Model Identifiers");

    int index = m_identifiersArgument->identifiers().indexOf("Id");
    if(index > -1)
    {
      m_identifiersArgument->setValue(&index , id());
    }

    index = m_identifiersArgument->identifiers().indexOf("Caption");
    if(index > -1)
    {
      m_identifiersArgument->setValue(&index , caption());
    }

    index = m_identifiersArgument->identifiers().indexOf("Description");
    if(index > -1)
    {
      m_identifiersArgument->setValue(&index , description());
    }

    m_identifiersArgument->addInputFileTypeFilter("Input XML File (*.xml)");
    m_identifiersArgument->setMatchIdentifiersWhenReading(true);

    addArgument(m_identifiersArgument);
  }

  //Input files
  {
    Dimension *inputFileDimension = new Dimension("Input File Dimension","",this);
    QStringList fidentifiers;
    fidentifiers.append("Input File");
    fidentifiers.append("Output File");
    fidentifiers.append("Report File");
    Quantity* fquantity = Quantity::unitLessValues("InputFilesUnit", "", QVariant::String, this);
    m_inputFilesArgument = new IdBasedArgumentQString("InputFiles", fidentifiers,inputFileDimension,fquantity,this);
    m_inputFilesArgument->setCaption("Model Input Files");
    m_inputFilesArgument->addInputFileTypeFilter("Input XML File (*.xml)");
    m_inputFilesArgument->setMatchIdentifiersWhenReading(true);

    addArgument(m_inputFilesArgument);
  }


  //Node exchange items
  {
    Quantity *exchangeItemQuantity = Quantity::unitLessValues("ExchangeItemUnit","", QVariant::String , this);

    Dimension *nodeExchangeDimension = new Dimension("NodeExchangeItemsDimension","", this);
    m_usedNodesArgument = new Argument1DString("NodeExchangeItems",nodeExchangeDimension,1,exchangeItemQuantity,this);
    m_usedNodesArgument->setCaption("Node Exchange Items");
    m_usedNodesArgument->addInputFileTypeFilter("Input XML File (*.xml)");
    addArgument(m_usedNodesArgument);

    Dimension *linksExchangeDimension = new Dimension("LinksExchangeItemsDimension","", this);
    m_usedLinksArgument = new Argument1DString("LinkExchangeItems",linksExchangeDimension,1,exchangeItemQuantity,this);
    m_usedLinksArgument->setCaption("Link Exchange Items");
    m_usedLinksArgument->addInputFileTypeFilter("Input XML File (*.xml)");
    addArgument(m_usedLinksArgument);

    Dimension *subCatchmentsExchangeDimension = new Dimension("SubCatchmentsExchangeItemsDimension","", this);
    m_usedSubCatchmentsArgument = new Argument1DString("SubCatchmentsExchangeItems", subCatchmentsExchangeDimension,1,exchangeItemQuantity,this);
    m_usedSubCatchmentsArgument->setCaption("SubCatchment Exchange Items");
    m_usedSubCatchmentsArgument->addInputFileTypeFilter("Input XML File (*.xml)");
    addArgument(m_usedSubCatchmentsArgument);
  }
}

void SWMMComponent::createAdaptedOutputFactories()
{
   TemporalInterpolationFactory* tempInterpFactory = new TemporalInterpolationFactory("TemporalInterpolationFactory",this);
   addAdaptedOutputFactory(tempInterpFactory);
}

void SWMMComponent::initializeSWMMProject()
{
  disposeSWMMProject();

  QString inputFile = m_inputFiles["InputFile"].absoluteFilePath();
  char *inpF = new char[inputFile.length() + 1] ;
  std::strcpy (inpF, inputFile.toStdString().c_str());

  QString outputFile = m_inputFiles["OutputFile"].absoluteFilePath();
  char *inpO = new char[outputFile.length() + 1] ;
  std::strcpy (inpO, outputFile.toStdString().c_str());

  QString reportFile = m_inputFiles["ReportFile"].absoluteFilePath();
  char *inpR = new char[reportFile.length() + 1] ;
  std::strcpy (inpR, reportFile.toStdString().c_str());

  m_SWMMProject = swmm_open(inpF,inpR,inpO);

  delete[] inpF;
  delete[] inpO;
  delete[] inpR;

  int y, m, d, h, mm, s;
  datetime_decodeDateTime(m_SWMMProject->StartDateTime, &y,&m,&d,&h,&mm,&s);
  m_startDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));
  m_currentDateTime->setDateTime(m_startDateTime->qDateTime());

  datetime_decodeDateTime(m_SWMMProject->EndDateTime, &y,&m,&d,&h,&mm,&s);
  m_endDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));
}

void SWMMComponent::disposeSWMMProject()
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
  else
  {
    qDebug() << "";
  }

}

Project *SWMMComponent::SWMMProject() const
{
  return m_SWMMProject;
}

Time* SWMMComponent::startDateTime() const
{
  return m_startDateTime;
}

Time* SWMMComponent::endDateTime() const
{
  return m_endDateTime;
}

Time* SWMMComponent::currentDateTime() const
{
  return m_currentDateTime;
}

void SWMMComponent::initialize()
{
  if(status() == HydroCouple::Created)
  {
    setStatus(HydroCouple::Initializing , "Initializing SWMM Model");

    QString message;

    if(initializeArguments(message))
    {
      initializeSWMMProject();

      if(!hasError(message))
      {
        clearInputExchangeItems();
        clearOutputExchangeItems();

        initializeNodeInputExchangeItems();
        initializeLinkInputExchangeItems();
        initializeSubCatchmentInputExchangeItems();

        initializeNodeOutputExchangeItems();
        initializeLinkOutputExchangeItems();
        initializeSubCatchmentOutputExchangeItems();

        setStatus(HydroCouple::Initialized , "Initialized SWMM Model");
        m_initialized = true;
      }
      else
      {
        setStatus(HydroCouple::Failed , message);
        m_initialized = false;
      }
    }
    else
    {
      setStatus(HydroCouple::Failed , message);
      m_initialized = false;
    }
  }
  else
  {
    //throw exception here.
    m_initialized = false;
  }
}

bool SWMMComponent::initializeArguments(QString &message)
{
  m_inputFiles.clear();
  int stride = 1;

  //Identifiers
  {
    int index = m_identifiersArgument->identifiers().indexOf("Id");
    if(index > -1)
    {
      QString identifier;
      m_identifiersArgument->getValues(&index, &stride, &identifier);
      setId(identifier);
    }

    index = m_identifiersArgument->identifiers().indexOf("Caption");
    if(index > -1)
    {
      QString caption;
      m_identifiersArgument->getValues(&index, &stride, &caption);
      setCaption(caption);
    }

    index = m_identifiersArgument->identifiers().indexOf("Description");
    if(index > -1)
    {
      QString description;
      m_identifiersArgument->getValues(&index, &stride, &description);
      setDescription(description);
    }
  }

  //input files
  {

    int index = m_inputFilesArgument->identifiers().indexOf("Input File");
    if(index > -1)
    {
      QString inputFilePath;
      m_inputFilesArgument->getValues(&index, &stride, &inputFilePath);
      QFileInfo inputFile(inputFilePath);

      if(!inputFile.exists())
      {
        message = "Input file does not exist: " + inputFile.absoluteFilePath();
        return false;
      }
      else
      {
        m_inputFiles["InputFile"] = inputFile;
      }
    }
    else
    {
      message = "Input file has not been specified";
      return false;
    }

    index = m_inputFilesArgument->identifiers().indexOf("Output File");
    if(index > -1)
    {
      QString outputFilePath;
      m_inputFilesArgument->getValues(&index, &stride, &outputFilePath);
      QFileInfo outputFile(outputFilePath);

      if(!outputFile.absoluteDir().exists())
      {
        message = "Output file directory does not exist: " + outputFile.absolutePath();
        return false;
      }
      else
      {
        m_inputFiles["OutputFile"] = outputFile;
      }
    }
    else
    {
      message = "Output file has not been specified";
      return false;
    }

    index = m_inputFilesArgument->identifiers().indexOf("Report File");
    if(index > -1)
    {
      QString reportFilePath;
      m_inputFilesArgument->getValues(&index, &stride, &reportFilePath);
      QFileInfo reportFile(reportFilePath);

      if(!reportFile.absoluteDir().exists())
      {
        message = "Report file directory does not exist: " + reportFile.absolutePath();
        return false;
      }
      else
      {
        m_inputFiles["ReportFile"] = reportFile;
      }
    }
    else
    {
      message = "Report file has not been specified";
      return false;
    }
  }

  //Objects to expose as exchangeitems
  {
    int length;
    if((length = m_usedNodesArgument->dimensionLength(nullptr,0)))
    {
      m_usedNodes.clear();
      std::vector<QString> values(length);

      int index = 0;
      m_usedNodesArgument->getValues(&index,&length,values.data());

      for(int i = 0 ; i < length; i++)
      {
        m_usedNodes.append(values[i]);
      }

    }

    if((length = m_usedLinksArgument->dimensionLength(nullptr,0)))
    {
      m_usedLinks.clear();
      std::vector<QString> values(length);

      int index = 0;
      m_usedLinksArgument->getValues(&index,&length,values.data());

      for(int i = 0 ; i < length; i++)
      {
        m_usedLinks.append(values[i]);
      }
    }

    if((length = m_usedSubCatchmentsArgument->dimensionLength(nullptr,0)))
    {
      m_usedSubCatchments.clear();
      std::vector<QString> values(length);

      int index = 0;
      m_usedSubCatchmentsArgument->getValues(&index,&length,values.data());

      for(int i = 0 ; i < length; i++)
      {
        m_usedSubCatchments.append(values[i]);
      }
    }
  }

  return true;
}

void SWMMComponent::initializeNodeInputExchangeItems()
{
  if(m_usedNodes.count())
  {
    int numNodes = m_usedNodes.length();

    Unit *waterSurfaceElevationUnit = Unit::lengthInFeet(this);
    waterSurfaceElevationUnit->setCaption("Elevation (ft)");
    Quantity *waterSurfaceElevation = new Quantity("Water Surface Elevation (ft)", QVariant::Double, waterSurfaceElevationUnit, this);

    Unit *lateralInflowUnit = Unit::flowInCFS(this);
    Quantity *lateralInflow = new Quantity("Flow (m³/s)" , QVariant::Double,lateralInflowUnit,this);

    for(int i = 0 ; i <  numNodes; i++)
    {
      char *nodeId = new char[m_usedNodes[i].length() + 1] ;
      std::strcpy (nodeId, m_usedNodes[i].toStdString().c_str());

      int index = project_findObject(m_SWMMProject , NODE , nodeId);

      if(index >= 0)
      {
        SWMMNodeWSETimeSeriesInput *inputWSEItem = new SWMMNodeWSETimeSeriesInput(&m_SWMMProject->Node[index],
                                                                                  new Dimension(QString(m_SWMMProject->Node[index].ID) + " Time Dimension" , "", this),
                                                                                  QList<Time*>({new Time(m_startDateTime->qDateTime())}),
                                                                                  waterSurfaceElevation,
                                                                                  this);
        inputWSEItem->setCaption(" Water Surface Elevation (ft) - " + QString(m_SWMMProject->Node[index].ID) );
        addInputExchangeItem(inputWSEItem);

        SWMMNodeLatInflowTimeSeriesInput *inputLatInfItem = new SWMMNodeLatInflowTimeSeriesInput(&m_SWMMProject->Node[index],
                                                                                                 new Dimension(QString(m_SWMMProject->Node[index].ID) + " Time Dimension" , "" , this),
                                                                                                 QList<Time*>({new Time(m_startDateTime->qDateTime())}),
                                                                                                 lateralInflow ,
                                                                                                 this);
        inputLatInfItem->setCaption("Lateral Inflow (m³/s) - " + QString(m_SWMMProject->Node[index].ID));
        addInputExchangeItem(inputLatInfItem);
      }

      delete[] nodeId;
    }
  }
}

void SWMMComponent::initializeLinkInputExchangeItems()
{

}

void SWMMComponent::initializeSubCatchmentInputExchangeItems()
{

}

void SWMMComponent::initializeNodeOutputExchangeItems()
{
  //Water Surface Elevation
  if(m_usedNodes.count())
  {
    int numNodes = m_usedNodes.length();

    Unit* waterSurfaceElevationUnit = Unit::lengthInFeet(this);
    waterSurfaceElevationUnit->setCaption("Elevation (ft)");
    Quantity* waterSurfaceElevation = new Quantity("Water Surface Elevation (ft)", QVariant::Double, waterSurfaceElevationUnit, this);

    for(int i = 0 ; i <  numNodes; i++)
    {
      char *nodeId = new char[m_usedNodes[i].length() + 1] ;
      std::strcpy (nodeId, m_usedNodes[i].toStdString().c_str());

      int index = project_findObject(m_SWMMProject , NODE , nodeId);

      if(index >= 0)
      {
        SWMMNodeWSETimeSeriesOutput *outputItem = new SWMMNodeWSETimeSeriesOutput(&m_SWMMProject->Node[index],
                                                                                  new Dimension(QString(m_SWMMProject->Link[index].ID) + " Time Dimension" , "", this),
                                                                                  QList<Time*>({new Time(m_startDateTime->qDateTime())}),
                                                                                  waterSurfaceElevation,
                                                                                  this);
        outputItem->setCaption("Water Surface Elevation (ft) - "+ QString(m_SWMMProject->Node[index].ID));
        addOutputExchangeItem(outputItem);
      }

      delete[] nodeId;
    }
  }
}

void SWMMComponent::initializeLinkOutputExchangeItems()
{
  //Water Surface Elevation
  if(m_usedLinks.count())
  {
    int numLinks = m_usedLinks.length();

    Unit *flowUnit = Unit::flowInCFS(this);
    Quantity *flow = new Quantity("Discharge (cfs)" , QVariant::Double,flowUnit,this);

    for(int i = 0 ; i <  numLinks; i++)
    {
      char *linkId = new char[m_usedLinks[i].length() + 1] ;
      std::strcpy (linkId, m_usedLinks[i].toStdString().c_str());

      int index = project_findObject(m_SWMMProject , LINK , linkId);

      if(index >= 0)
      {
        SWMMLinkDischargeTimeSeriesOutput *outputItem = new SWMMLinkDischargeTimeSeriesOutput(&m_SWMMProject->Link[index],
                                                                                              new Dimension(QString(m_SWMMProject->Link[index].ID) + " Time Dimension" , "" , this),
                                                                                              QList<Time*>({new Time(m_startDateTime->qDateTime())}),
                                                                                              flow ,
                                                                                              this);
        outputItem->setCaption("Discharge (m³/s) - "+QString(m_SWMMProject->Link[index].ID));
        addOutputExchangeItem(outputItem);
      }

      delete[] linkId;
    }
  }
}

void SWMMComponent::initializeSubCatchmentOutputExchangeItems()
{

}

bool SWMMComponent::hasError(QString &message)
{
  int error = m_SWMMProject->ErrCode;

  if(error)
  {
    message = QString(getErrorMsg(m_SWMMProject->ErrCode)).trimmed();
    return true;
  }
  else
  {
    return false;
  }
}

HydroCouple::IModelComponent* SWMMComponent::clone()
{
  return nullptr;

}

QList<QString> SWMMComponent::validate()
{
  if(m_initialized)
  {
    setStatus(HydroCouple::Validating,"Validating SWMM Model");

    //check connections

    setStatus(HydroCouple::Valid,"SWMM Model Is Valid");


  }
  else
  {
    //throw has not been initialized yet.
  }

  return QList<QString>();
}

void SWMMComponent::prepare()
{
  if(m_initialized)
  {
    setStatus(HydroCouple::Preparing , "Preparing SWMM Model");

    m_usedInputs.clear();

    QList<HydroCouple::IInput*> inputExchangeItems = inputs();

    for(HydroCouple::IInput *input : inputExchangeItems)
    {
      HydroCouple::IMultiInput* minput  = dynamic_cast<HydroCouple::IMultiInput*>(input);

      if((minput && minput->providers().length()) || input->provider())
      {
        SWMMInputObjectItem* inputObject = dynamic_cast<SWMMInputObjectItem*>(input);
        m_usedInputs.append(inputObject);
      }
    }

    m_usedOutputs.clear();

    QList<HydroCouple::IOutput*> outputExchangeItems = outputs();

    for(HydroCouple::IOutput *output : outputExchangeItems)
    {
      if(output->consumers().length() || output->adaptedOutputs().length())
      {
        SWMMOutputObjectItem *outpuObject = dynamic_cast<SWMMOutputObjectItem*>(output);
        m_usedOutputs.append(outpuObject);

        if(output->adaptedOutputs().length())
        {
          for(HydroCouple::IAdaptedOutput* adaptedOutput : output->adaptedOutputs())
          {
            adaptedOutput->initialize();
          }
        }
      }

    }

    swmm_start(m_SWMMProject, TRUE);

    if (m_SWMMProject->ErrorCode)
    {
      QString message;
      hasError(message);
      setStatus(HydroCouple::Failed , message);
      return;
    }

    setStatus(HydroCouple::Updated ,"Finished preparing SWMM Model");
    m_prepared = true;
  }
  else
  {
    m_prepared = false;
    //throw exceptions
  }
}

void SWMMComponent::update(const QList<HydroCouple::IOutput *> &requiredOutputs)
{
  if(status() == HydroCouple::Updated)
  {
    setStatus(HydroCouple::Updating , "SWMM simulation with component id " + id() + " is performing time-step" , m_currentProgress);

    updateInputExchangeItems();

    DateTime elapsedTime = 0;
    swmm_step(m_SWMMProject,&elapsedTime);

    int y, m, d, h, mm, s;
    datetime_decodeDateTime(m_SWMMProject->StartDateTime + elapsedTime, &y,&m,&d,&h,&mm,&s);
    m_currentDateTime->setDateTime(QDateTime(QDate(y,m,d),QTime(h,mm,s)));

    if(requiredOutputs.length())
    {
      updateOutputExchangeItems(requiredOutputs);
    }
    else
    {
      updateOutputExchangeItems();
    }

    QString errMessage;

    if(elapsedTime <= 0 && !m_SWMMProject->ErrCode)
    {
      setStatus(HydroCouple::Done , "SWMM simulation with component id " + id() + " finished successfully",100);
    }
    else if(hasError(errMessage))
    {
      setStatus(HydroCouple::Failed ,  "SWMM simulation with component id " + id() + " failed with error message : " + errMessage);
    }
    else
    {
      double progress = (elapsedTime) * 100.0
                        / (m_SWMMProject->EndDateTime - m_SWMMProject->StartDateTime);

      m_currentProgress = (int) progress;
      setStatus(HydroCouple::Updated , "SWMM simulation with component id " + id() + " performed time-step to " + m_currentDateTime->qDateTime().toString(Qt::ISODate) , m_currentProgress);
    }
  }
  else
  {
    qDebug() << "";
  }
}

void SWMMComponent::updateInputExchangeItems()
{
  for(SWMMInputObjectItem *input : m_usedInputs)
  {
    input->retrieveOuputItemData();
  }
}

void SWMMComponent::updateOutputExchangeItems()
{
  for(SWMMOutputObjectItem *output : m_usedOutputs)
  {
    output->retrieveDataFromModel();

    HydroCouple::IOutput* toutput = dynamic_cast<HydroCouple::IOutput*>(output);

    if(toutput)
    {
      for(HydroCouple::IAdaptedOutput* adaptedOutput : toutput->adaptedOutputs())
      {
        adaptedOutput->refresh();
      }
    }
  }
}

void SWMMComponent::updateOutputExchangeItems(const QList<HydroCouple::IOutput *> &requiredOutputs)
{
  for(HydroCouple::IOutput *output : requiredOutputs)
  {
    SWMMOutputObjectItem *outputItem = dynamic_cast<SWMMOutputObjectItem*>(output);

    if(outputItem)
    {
      outputItem->retrieveDataFromModel();
    }
  }
}

void SWMMComponent::finish()
{
  if(m_prepared)
  {
    setStatus(HydroCouple::Finishing , "SWMM simulation with component id " + id() + " is being disposed" , 100);

    clearInputExchangeItems();
    clearOutputExchangeItems();

    disposeSWMMProject();

    m_prepared = false;
    m_initialized = false;

    setStatus(HydroCouple::Finished , "SWMM simulation with component id " + id() + " has been disposed" , 100);
    setStatus(HydroCouple::Created , "SWMM simulation with component id " + id() + " ran successfully and has been re-created" , 100);

  }
}
