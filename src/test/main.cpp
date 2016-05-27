#include "stdafx.h"
#include <QtTest>
#include "swmmcomponenttest.h"

int main(int argc, char *argv[])
{
   int status = 0;
   {
      SWMMComponentTest test;
      status |= QTest::qExec(&test, argc, argv);
   }

   return status;
}
