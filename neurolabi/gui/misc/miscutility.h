#ifndef MISCUTILITY_H
#define MISCUTILITY_H

#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <iostream>

#include "zhistogram.h"
#include "zobject3dscan.h"
#include "neutube_def.h"
#include "zintcuboidarray.h"
#include "zpointarray.h"
#include "ztree.h"
#include "zarray.h"

class ZGraph;
class ZIntPoint;
class ZSwcTree;
class ZClosedCurve;
class ZIntCuboid;
class ZCuboid;

namespace misc {

void paintRadialHistogram(const ZHistogram hist, double cx, double cy, int z,
                          Stack *stack);
void paintRadialHistogram2D(const std::vector<ZHistogram> hist,
                            double cx, int startZ, Stack *stack);

/*!
 * \brief Y normal of a binary stack
 */
Stack* computeNormal(const Stack *stack, neutube::EAxis axis);

int computeRavelerHeight(const ZIntCuboidArray &blockArray, int margin);

bool exportPointList(const std::string &filePath, const ZPointArray &pointArray);

ZGraph* makeCoOccurGraph(const Stack *stack, int nnbr);

ZIntPoint getDsIntvFor3DVolume(const ZIntCuboid &box);

ZIntPoint getDsIntvFor3DVolume(double dsRatio);
int getIsoDsIntvFor3DVolume(double dsRatio, bool powed);
int getIsoDsIntvFor3DVolume(
    const ZIntCuboid &box, size_t maxVolume, bool powed);

int GetZoomScale(int zoom);
//int GetZoomLevel(int maxLevel, int width, int height, int zoom);

double GetExpansionScale(size_t currentVol, size_t maxVol);

/*!
 * \brief A function for computing confidence
 *
 * \param median The value where confidence = 0.5
 * \param p95 The value where confidence = 0.95
 */
double computeConfidence(double v, double median, double p95);

ZTree<int> *buildSegmentationTree(const Stack *stack);

ZClosedCurve convertSwcToClosedCurve(const ZSwcTree &tree);

ZCuboid Intersect(const ZCuboid &box1, const ZIntCuboid &box2);
ZCuboid CutBox(const ZCuboid &box1, const ZIntCuboid &box2);

ZIntCuboid GetBoundBox(const ZArray *array);

enum ESampleStackOption {
  SAMPLE_STACK_NN, SAMPLE_STACK_AVERAGE, SAMPLE_STACK_UNIFORM
};

double SampleStack(const Stack *stack, double x, double y, double z,
                   ESampleStackOption option);

template<typename T>
void read(std::istream &stream, T &v)
{
  stream.read((char*)(&v), sizeof(T));
}

template<typename T>
void read(std::istream &stream, T &v, size_t n)
{
  stream.read((char*)(&v), sizeof(T) * n);
}

template<typename T>
void write(std::ostream &stream, const T &v)
{
  stream.write((const char*)(&v), sizeof(T));
}

template<typename T>
void write(std::ostream &stream, const T *v, size_t n)
{
  stream.write((const char*)(v), sizeof(T) * n);
}

template<typename T>
void assign(T *out, const T &v);

/*!
 * \brief Parse hdf5 path
 *
 * "file.h5f:/group/path"
 *
 * \return empty array if the parsing failed
 */
std::vector<std::string> parseHdf5Path(const std::string &path);

template<typename T>
std::set<T> intersect(const std::set<T> &s1, const std::set<T> &s2);

template<typename T>
std::set<T> setdiff(const std::set<T> &s1, const std::set<T> &s2);
}

// generic solution
template <class T>
int numDigits(T number)
{
  int digits = 0;
  if (number < 0) digits = 1; // remove this line if '-' counts as a digit
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

//template<>
//int numDigits(int32_t x);

template<typename T>
std::set<T> misc::intersect(const std::set<T> &s1, const std::set<T> &s2)
{
  std::set<T> result;
  std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::inserter(result, result.begin()));
  return result;
}

template<typename T>
std::set<T> misc::setdiff(const std::set<T> &s1, const std::set<T> &s2)
{
  std::set<T> result;
  std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(),
                      std::inserter(result, result.begin()));
  return result;
}

template<typename T>
void misc::assign(T *out, const T &v)
{
  if (out != NULL) {
    *out = v;
  }
}

//// partial-specialization optimization for 8-bit numbers
//template <>
//int numDigits(char n)
//{
//  // if you have the time, replace this with a static initialization to avoid
//  // the initial overhead & unnecessary branch
//  static char x[256] = {0};
//  if (x[0] == 0) {
//    for (char c = 1; c != 0; c++)
//      x[static_cast<int>(c)] = numDigits((int32_t)c);
//    x[0] = 1;
//  }
//  return x[static_cast<int>(n)];
//}


#endif // MISCUTILITY_H
