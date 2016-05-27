#ifndef SWMMNODETIMESERIESEXCHANGEITEM
#define SWMMNODETIMESERIESEXCHANGEITEM

#include "swmmcomponent_global.h"
#include "core/headers.h"
#include "temporal/timeseriesexchangeitem.h"
#include "swmmobjectitems.h"

class SWMMComponent ;


class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesOutput : public Temporal::TimeSeriesOutputDouble , public virtual SWMMOutputObjectItem
{
      Q_OBJECT

   public:

      SWMMNodeWSETimeSeriesOutput(TNode* node, ValueDefinition *valueDefinition, SWMMComponent *component);

      ~SWMMNodeWSETimeSeriesOutput();

      void update(HydroCouple::IInput *querySpecifier) override;

      void retrieveDataFromModel() override;

   private:
      TNode *m_node;
      SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeWSETimeSeriesInput : public Temporal::TimeSeriesInputDouble, public virtual SWMMInputObjectItem
{
      Q_OBJECT

   public:

      SWMMNodeWSETimeSeriesInput(TNode* node, ValueDefinition *valueDefinition, SWMMComponent *component);

      ~SWMMNodeWSETimeSeriesInput();

      void retrieveOuputItemData() override;

   private:
      TNode *m_node;
      SWMMComponent *m_component;

};

class SWMMCOMPONENT_EXPORT SWMMNodeLatInflowTimeSeriesInput : public Temporal::TimeSeriesMultiInputDouble, public virtual SWMMInputObjectItem
{
     Q_OBJECT

   public:

      SWMMNodeLatInflowTimeSeriesInput(TNode* node, ValueDefinition *valueDefinition, SWMMComponent *component);

      ~SWMMNodeLatInflowTimeSeriesInput();

      void retrieveOuputItemData() override;

   private:
      TNode *m_node;
      SWMMComponent *m_component;
};


class SWMMCOMPONENT_EXPORT SWMMLinkDischargeTimeSeriesOutput : public Temporal::TimeSeriesOutputDouble, public virtual SWMMOutputObjectItem
{
      Q_OBJECT

   public:

      SWMMLinkDischargeTimeSeriesOutput(TLink* link, ValueDefinition *valueDefinition, SWMMComponent *component);

      ~SWMMLinkDischargeTimeSeriesOutput();

      void update(HydroCouple::IInput *querySpecifier) override;

      void retrieveDataFromModel() override;

   private:
      TLink *m_link;
      SWMMComponent *m_component;
};



#endif // SWMMNODETIMESERIESEXCHANGEITEM

