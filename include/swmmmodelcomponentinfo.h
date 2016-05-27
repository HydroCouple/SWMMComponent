#ifndef SWMMMODELCOMPONENTINFO_H
#define SWMMMODELCOMPONENTINFO_H

#include "swmmcomponent_global.h"
#include "modelcomponentinfo.h"

class SWMMCOMPONENT_EXPORT SWMMModelComponentInfo : public ModelComponentInfo, public virtual HydroCouple::IModelComponentInfo
{
      Q_OBJECT
      Q_PLUGIN_METADATA(IID "SWMMModelComponentInfo")
      Q_INTERFACES(HydroCouple::IModelComponentInfo)

   public:
      SWMMModelComponentInfo(QObject *parent = nullptr);

      virtual ~SWMMModelComponentInfo();

      HydroCouple::IModelComponent* createComponentInstance() override;

};

Q_DECLARE_METATYPE(SWMMModelComponentInfo*)

#endif // SWMMMODELCOMPONENTINFO_H
