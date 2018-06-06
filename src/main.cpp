#include "stdafx.h"
#include <QFileInfo>
#include "spatial/geometryfactory.h"
#include "spatial/linestring.h"
#include "spatial/point.h"
#include "spatial/envelope.h"
#include "cmath"

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <unordered_map>

struct PrjNode
{
    std::string id;
    HCPoint *point;
};

struct CrossSection
{
    std::string id;
    std::vector<double> stations;
    std::vector<double> elevations;
    HCLineString *lineString;
};

struct PrjLink
{
    std::string id;
    std::string upNode, downNode;
    std::string crossSectionId;
    HCLineString *lineString;
};

bool compare(HCPoint *p1, HCPoint *p2, double eps = 1e-6)
{
  double diff = hypot(p1->x() - p2->x(), p1->y() - p2->y());

  return diff <= eps;
}

void createSWMMProject(int argc, char*argv[])
{
  //Nodes
  //Links
  //Cross-sections

  if(argc > 4)
  {
    GDALAllRegister();
    QDir dir(QDir::currentPath());

    QFileInfo inputAlignmentFile(argv[1]);
    QFileInfo inputNodesFile(argv[2]);
    QFileInfo inputXSectionFile(argv[3]);
    QFileInfo outputFileXSection(argv[4]);

    if(inputAlignmentFile.isRelative())
      inputAlignmentFile = QFileInfo(dir.absoluteFilePath(inputAlignmentFile.filePath()));

    if(inputNodesFile.isRelative())
      inputNodesFile = QFileInfo(dir.absoluteFilePath(inputNodesFile.filePath()));

    if(inputXSectionFile.isRelative())
      inputXSectionFile = QFileInfo(dir.absoluteFilePath(inputXSectionFile.filePath()));

    if(outputFileXSection.isRelative())
      outputFileXSection = QFileInfo(dir.absoluteFilePath(outputFileXSection.filePath()));


    if(inputAlignmentFile.isFile() && inputAlignmentFile.exists() &&
       inputNodesFile.isFile() && inputNodesFile.exists() &&
       inputXSectionFile.isFile() && inputXSectionFile.exists())
    {
      QFile outFile(outputFileXSection.absoluteFilePath());

      if(outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
        std::unordered_map<std::string,PrjNode*> nodes;
        std::unordered_map<std::string,CrossSection*> crossSections;
        std::unordered_map<std::string,PrjLink*> links;

        QTextStream writer(&outFile);

        Envelope extent;
        QString errorMessage;
        QList<HCGeometry*> nodeGeoms;
        QList<HCGeometry*> linkGeoms;
        QList<HCGeometry*> xSectGeoms;

        GeometryFactory::readGeometryFromFile(inputNodesFile.absoluteFilePath(), nodeGeoms, extent, errorMessage);
        GeometryFactory::readGeometryFromFile(inputAlignmentFile.absoluteFilePath(), linkGeoms, extent, errorMessage);
        GeometryFactory::readGeometryFromFile(inputXSectionFile.absoluteFilePath(), xSectGeoms, extent, errorMessage);

        QString zero = "0";

        if(nodeGeoms.size())
        {

          writer << "[JUNCTIONS]" << endl;
          writer << ";;               Invert     Max.       Init.      Surcharge  Ponded" << endl;
          writer << ";;Name           Elev.      Depth      Depth      Depth      Area" << endl;
          writer << ";;-------------- ---------- ---------- ---------- ---------- ----------" << endl;


          for(int i = 0; i < nodeGeoms.size(); i++)
          {
            PrjNode *node = new PrjNode;
            node->id =  "J_" + std::to_string(i);
            node->point = dynamic_cast<HCPoint*>(nodeGeoms[i]);
            QString elev = QString::number(node->point->z(),'g', 16);
            writer << QString::fromStdString(node->id).leftJustified(17) << elev.leftJustified(12) << zero.leftJustified(11)
                   << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << endl;
            nodes[node->id] = node;
          }


          if(linkGeoms.size())
          {
            writer << "" << endl;
            writer << "[CONDUITS]" << endl;
            writer << ";;               Inlet            Outlet                      Manning    Inlet      Outlet     Init.      Max." << endl;
            writer << ";;Name           Node             Node             Length     N          Offset     Offset     Flow       Flow" << endl;
            writer << ";;-------------- ---------------- ---------------- ---------- ---------- ---------- ---------- ---------- ----------" << endl;

            QString mannings = "0.04";

            for(int i = 0; i < linkGeoms.size(); i++)
            {
              HCLineString *lineString = dynamic_cast<HCLineString*>(linkGeoms[i]);
              HCPoint *lb = lineString->pointInternal(0);
              HCPoint *le = lineString->pointInternal(lineString->pointCount() - 1);

              PrjNode *from = nullptr;
              PrjNode *to  = nullptr;

              for(const std::pair<std::string, PrjNode*> &node : nodes)
              {
                if(compare(node.second->point, lb))
                {
                  from = node.second;
                }
                else if(compare(node.second->point, le))
                {
                  to = node.second;
                }

                if(from && to)
                {
                  break;
                }
              }


              if(from && to)
              {
                PrjLink *link = new PrjLink;
                link->id = "L_" + std::to_string(i);
                link->upNode = from->id;
                link->downNode = to->id;
                link->lineString = lineString;

                links[link->id] = link;

                writer << QString::fromStdString(link->id).leftJustified(17) << QString::fromStdString(link->upNode).leftJustified(17)
                       << QString::fromStdString(link->downNode).leftJustified(17) << QString::number(lineString->length()).leftJustified(11) << mannings.leftJustified(11)
                       << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << endl;
              }
            }


            if(xSectGeoms.size())
            {
              for(int i = 0; i < xSectGeoms.size(); i++)
              {
                HCLineString *lineString = dynamic_cast<HCLineString*>(xSectGeoms[i]);
                std::string id = "T_" + std::to_string(i);
                int count = 0;

                for(const std::pair<std::string, PrjLink*> &link : links)
                {
                  if(link.second->lineString->intersects(lineString))
                  {
                    link.second->crossSectionId = id;
                    count ++;
                  }
                }

                if(count)
                {
                  CrossSection *xsect = new CrossSection;
                  xsect->id = id;

                  double ce = 0;
                  HCPoint *cp = lineString->pointInternal(0);

                  xsect->stations.push_back(ce);
                  xsect->elevations.push_back(cp->z());

                  for(int j = 1; j < lineString->pointCount() ; j++)
                  {
                    HCPoint *np = lineString->pointInternal(j);
                    ce += hypot(cp->x() - np->x() , cp->y() - np->y());

                    xsect->stations.push_back(ce);
                    xsect->elevations.push_back(np->z());

                    cp = np;
                  }

                  crossSections[id] = xsect;
                }
              }
            }

            writer << "" << endl;
            writer << "[XSECTIONS]" << endl;
            writer << ";;Link           Shape        Geom1            Geom2      Geom3      Geom4      Barrels" << endl;
            writer << ";;-------------- ------------ ---------------- ---------- ---------- ---------- -------" << endl;

            for(const std::pair<std::string, PrjLink*> &link : links)
            {
              PrjLink *iLink = link.second;

              if(!QString::fromStdString(iLink->crossSectionId).isEmpty() && !QString::fromStdString(iLink->crossSectionId).isNull())
              {
                writer << QString::fromStdString(iLink->id).leftJustified(17) << QString("IRREGULAR").leftJustified(13) << QString::fromStdString(iLink->crossSectionId).leftJustified(13) << endl;

              }
              else
              {
                writer << QString::fromStdString(iLink->id).leftJustified(17) << QString("RECT_OPEN").leftJustified(13) << QString::number(1.5).leftJustified(17) << QString::number(8.0).leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << QString("1").leftJustified(11) << zero.leftJustified(11) << endl;
              }
            }


            if(crossSections.size())
            {
              writer << "" << endl;
              writer << "[TRANSECTS]" << endl;
              writer << ";;;;Transect Data in HEC-2 format" << endl;
              writer << ";;-------------- ------------ ---------------- ---------- ---------- ---------- -------" << endl;


              for(const std::pair<std::string, CrossSection*> &xsectPair : crossSections)
              {
                CrossSection *xsect = xsectPair.second;
                writer << QString("NC").leftJustified(11) << mannings.leftJustified(11) <<  mannings.leftJustified(11) << mannings.leftJustified(11) << endl;
                writer << QString("X1").leftJustified(11) << QString::fromStdString(xsect->id).leftJustified(11)  << QString::number(xsect->stations.size()).leftJustified(11)
                       << zero.leftJustified(11) << zero.leftJustified(11)
                       << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11)
                       << zero.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << endl;

                writer << QString("GR").leftJustified(11);

                int numToWrite = 4;
                int count = 0;



                for(size_t k = 0; k < xsect->stations.size(); k++)
                {
                  writer  << QString::number(xsect->stations[k]).leftJustified(11);
                  writer  << QString::number(xsect->elevations[k]).leftJustified(11);

                  count ++;

                  if(count >= numToWrite)
                  {
                    numToWrite = 5;
                    count = 0;

                    writer << endl;

                    if(k < xsect->stations.size() -1)
                    {
                      writer << QString("GR").leftJustified(11);
                    }
                  }
                }

                writer << endl;
                writer << ";" << endl;
              }
            }
          }


          writer << "" << endl;
          writer << "[MAP]" << endl;
          writer << "DIMENSIONS " << extent.minX() << " " << extent.minY() << " " << extent.maxX() << " " << extent.maxY() << endl;// 1526037.495 3764743.440 1570678.941 3816874.269" << endl;
          writer << "Units      None" << endl;

          if(nodes.size())
          {
            writer << "" << endl;
            writer << "[COORDINATES]" << endl;
            writer << ";;Node           X-Coord            Y-Coord" << endl;
            writer << ";;-------------- ------------------ ------------------" << endl;

            for(const std::pair<std::string, PrjNode*> &node : nodes)
            {
              HCPoint *p = node.second->point;

              QString xN = QString::number(p->x(),'g',16);
              QString yN = QString::number(p->y(),'g',16);

              xN = xN.leftJustified(18) + " ";
              yN = yN.leftJustified(19);

              writer << QString::fromStdString(node.second->id).leftJustified(17) << xN << yN << endl;
            }
          }

          if(links.size())
          {
            writer << "" << endl;
            writer << "[VERTICES]" << endl;
            writer << ";;Link           X-Coord            Y-Coord" << endl;
            writer << ";;-------------- ------------------ ------------------" << endl;

            for(const std::pair<std::string, PrjLink*> &link : links)
            {
              HCLineString *lineString = link.second->lineString;

              for(int i = 1 ; i < lineString->pointCount() - 1 ; i++)
              {
                HCPoint *p = lineString->pointInternal(i);

                QString xN = QString::number(p->x(),'g',16);
                QString yN = QString::number(p->y(),'g',16);

                xN = xN.leftJustified(18) + " ";
                yN = yN.leftJustified(19);

                writer << QString::fromStdString(link.second->id).leftJustified(17) << xN << yN << endl;
              }
            }
          }

          qDeleteAll(nodeGeoms);
          qDeleteAll(linkGeoms);
          qDeleteAll(xSectGeoms);
        }
      }
    }
  }
}

void thinXSections(int argc, char*argv[])
{
  if(argc > 3)
  {
    GDALAllRegister();

    QDir dir(QDir::currentPath());

    QFileInfo inputXSectionFile(argv[1]);
    QFileInfo outputXSectionFile(argv[2]);

    QString minLimitString(argv[3]);
    bool parsedOk;
    double minLimit = minLimitString.toDouble(&parsedOk);

    if(!parsedOk)
      minLimit = 0.98;



    if(inputXSectionFile.isRelative())
      inputXSectionFile = QFileInfo(dir.absoluteFilePath(inputXSectionFile.filePath()));

    if(outputXSectionFile.isRelative())
      outputXSectionFile = QFileInfo(dir.absoluteFilePath(outputXSectionFile.filePath()));

    QList<HCGeometry*> polyLines;
    QList<HCGeometry*> outputPolyLines;
    QString error;
    Envelope envp;

    GeometryFactory::readGeometryFromFile(inputXSectionFile.absoluteFilePath(), polyLines, envp, error);

    double overallCount = 0;
    double reductionCount = 0;
    for(int i = 0; i < polyLines.size(); i++)
    {
      HCLineString *lineString = dynamic_cast<HCLineString*>(polyLines[i]);

      double beginCount = lineString->pointCount();
      overallCount += beginCount;


      if(lineString->pointCount() > 6)
      {
        QList<HCPoint*> pointsToRemove;

        do
        {
          pointsToRemove.clear();

          for(int j = 1; j < lineString->pointCount() - 1 ; j = j + 2)
          {
            Vect p1(*lineString->point(j-1));
            Vect p2(*lineString->point(j));
            Vect p3(*lineString->point(j+1));

            double dx1 = p1.v[0] - p2.v[0];
            double dy1 = p1.v[1] - p2.v[1];
            double l1 = -hypot(dx1, dy1);

            double dx2 = p3.v[0] - p2.v[0];
            double dy2 = p3.v[0] - p2.v[0];
            double l2 = hypot(dx2, dy2);


            p1.v[0] = -(p1.v[2] - p2.v[2]);
            p1.v[1] = 0.0;
            p1.v[2] = l1;

            p1.normalize();

            p2.v[0] = -(p3.v[2] - p2.v[2]);
            p2.v[1] = 0.0;
            p2.v[2] = l2;

            p2.normalize();

            double dotProd = fabs(Vect::dotProduct(p2, p1));

            if(dotProd >= minLimit)
            {
              pointsToRemove.push_back(lineString->pointInternal(j));
            }
          }

          for(HCPoint *point : pointsToRemove)
          {
            lineString->removePoint(point);
            delete point;
          }

        }while (pointsToRemove.size() && lineString->pointCount() > 6);
      }

      double endCount = lineString->pointCount();
      reductionCount += endCount;

      printf("Reduced point count from %f to %f -> (%f %%)\n", beginCount, endCount, (beginCount - endCount) * 100.0 / beginCount);

      outputPolyLines.push_back(lineString);
    }

    if(outputPolyLines.size())
    {
      GeometryFactory::writeGeometryToFile(outputPolyLines, "XSection", HydroCouple::Spatial::IGeometry::LineStringZ, "ESRI Shapefile", outputXSectionFile.absoluteFilePath(), error);
      printf("Reduced point count from %f to %f -> (%f %%)\n", overallCount, reductionCount, (overallCount - reductionCount) * 100.0 / overallCount);
    }

  }
}

int main(int argc, char* argv[])
{
   createSWMMProject(argc, argv);
  // thinXSections(argc, argv);

  return 0;
}
