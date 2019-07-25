#ifndef SWMMMODELCOMPONENTINFO_H
#define SWMMMODELCOMPONENTINFO_H

#include "swmmcomponent_global.h"
#include "core/abstractmodelcomponentinfo.h"

class SWMMCOMPONENT_EXPORT SWMMComponentInfo : public AbstractModelComponentInfo
{
      Q_OBJECT
      Q_PLUGIN_METADATA(IID "SWMMComponentInfo")

   public:
      SWMMComponentInfo(QObject *parent = nullptr);

      virtual ~SWMMComponentInfo() override;

      HydroCouple::IModelComponent* createComponentInstance() override;

      void createAdaptedOutputFactories();

};

Q_DECLARE_METATYPE(SWMMComponentInfo*)

#endif // SWMMMODELCOMPONENTINFO_H
