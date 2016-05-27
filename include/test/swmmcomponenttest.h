#ifndef SWMMCOMPONENTTEST_H
#define SWMMCOMPONENTTEST_H

#include <QObject>
#include "stdafx.h"
#include <QtTest/QtTest>
#include "swmmcomponent.h"

using namespace HydroCouple;

class SWMMComponentTest : public QObject
{
      Q_OBJECT

   private slots:
      void init()
      {

      }

      void readArgumentsFromFile()
      {
         SWMMComponent *component = new SWMMComponent("SWMMComponent",nullptr);

         for(IArgument* argument : component->arguments())
         {
            if(!argument->id().compare("InitializationArguments"))
            {
               SWMMComponentArgument* sargument = dynamic_cast<SWMMComponentArgument*>(argument);
               sargument->readValues("./../../examples/example1.xml" ,  true);
               qDebug() << sargument->toString();
            }
         }
         component->initialize();


         long newHour, oldHour = 0;
         long theDay, theHour;
         DateTime elapsedTime = 0.0;

         // --- run the simulation if input data OK
         if (!component->SWMMProject()->ErrorCode)
         {
            // --- initialize values
            swmm_start(component->SWMMProject(), TRUE);

            // --- execute each time step until elapsed time is re-set to 0
            if (!component->SWMMProject()->ErrorCode)
            {
               fprintf(stdout, "\n o  Simulating day: 0     hour:  0");
               fflush(stdout);

               do
               {
                  swmm_step(component->SWMMProject(), &elapsedTime);
                  newHour = (long)(elapsedTime * 24.0);
                  if (newHour > oldHour)
                  {
                     theDay = (long)elapsedTime;
                     theHour = (long)((elapsedTime - floor(elapsedTime)) * 24.0);
                     fprintf(stdout, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                     fflush(stdout);
                     sprintf(component->SWMMProject()->Msg, "%-5d hour: %-2d", theDay, theHour);
                     fprintf(stdout, component->SWMMProject()->Msg);
                     fflush(stdout);
                     oldHour = newHour;
                  }
               } while (elapsedTime > 0.0 && !component->SWMMProject()->ErrorCode);

               fprintf(stdout, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                               "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
               fflush(stdout);

               fprintf(stdout, "Simulation complete           ");
               fflush(stdout);
            }

            // --- clean up
            swmm_end(component->SWMMProject());
         }

         // --- report results
         if (component->SWMMProject()->Fout.mode == SCRATCH_FILE) swmm_report(component->SWMMProject());

         // --- close the system
         //swmm_close(project);
         //return project->ErrorCode;
         swmm_close(component->SWMMProject());




         QVERIFY(component->status() == HydroCouple::Initialized);


      }

      void cleanup()
      {

      }
};


#endif // SWMMCOMPONENTTEST_H

