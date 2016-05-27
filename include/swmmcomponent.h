#ifndef SWMMCOMPONENT_H
#define SWMMCOMPONENT_H

#include "swmmcomponent_global.h"
#include "headers.h"
#include "swmm5.h"
#include "abstractmodelcomponent.h"
#include "timedata.h"
#include "idbasedargument.h"

#include <QDateTime>

class SWMMModelComponentInfo;
class SWMMInputObjectItem;
class SWMMOutputObjectItem;

class SWMMCOMPONENT_EXPORT SWMMComponent : public AbstractModelComponent
{
      friend class SWMMModelComponentInfo;

      Q_OBJECT

   public:

      SWMMComponent(const QString &id, SWMMComponent* component = nullptr);

      SWMMComponent(const QString &id, const QString &caption, SWMMComponent* component = nullptr);

      virtual ~SWMMComponent();

      void createArguments();

      void initialize() override;

      void initializeSWMMProject();

      void disposeSWMMProject();

      Project* SWMMProject() const;
      
      HTime* startDateTime() const;
      
      HTime* endDateTime() const;
      
      HTime* currentDateTime() const;

      bool initializeArguments(QString &message);

      void initializeNodeInputExchangeItems();

      void initializeLinkInputExchangeItems();

      void initializeSubCatchmentInputExchangeItems();

      void initializeNodeOutputExchangeItems();

      void initializeLinkOutputExchangeItems();

      void initializeSubCatchmentOutputExchangeItems();

      bool hasError(QString &message);

      HydroCouple::IModelComponent* clone() override;

      QList<QString> validate() override;

      void prepare() override;

      void update(const QList<HydroCouple::IOutput*> &requiredOutputs = QList<HydroCouple::IOutput*>()) override;

      void updateInputExchangeItems();

      void updateOutputExchangeItems();

      void updateOutputExchangeItems(const QList<HydroCouple::IOutput *> &requiredOutputs);
      
      void finish() override;


   private:
      QList<SWMMInputObjectItem*> m_usedInputs;
      QList<SWMMOutputObjectItem*> m_usedOutputs;
      QStringList m_usedNodes, m_usedLinks, m_usedSubCatchments;
      SWMMModelComponentInfo *m_SWMMComponentInfo;
      IdBasedArgumentQString *m_identifiersArgument, *m_inputFilesArgument, *m_usedNodesArgument, *m_usedLinksArgument,*m_usedSubCatchmentsArgument;
      Project* m_SWMMProject;
      QHash<QString,QFileInfo> m_inputFiles;
      HTime *m_startDateTime, *m_endDateTime, *m_currentDateTime;
      bool m_initialized, m_prepared;
      int m_currentProgress;

};

Q_DECLARE_METATYPE(SWMMComponent*)

#endif // SWMMCOMPONENT_H
