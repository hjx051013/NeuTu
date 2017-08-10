#include "zmesh.h"

#include "zmeshio.h"
#include "zmeshutils.h"
#include "zbbox.h"
#include "zexception.h"
#include "QsLog.h"
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTubeFilter.h>
#include <vtkFloatArray.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <boost/math/constants/constants.hpp>
#include <map>

namespace {

ZMesh vtkPolyDataToMesh(vtkPolyData* polyData)
{
  CHECK(polyData);
  vtkPoints* points = polyData->GetPoints();
  vtkCellArray* polys = polyData->GetPolys();
  vtkDataArray* pointsNormals = polyData->GetPointData()->GetNormals();

  std::vector<glm::dvec3> vertices(points->GetNumberOfPoints());
  std::vector<glm::dvec3> normals(pointsNormals->GetNumberOfTuples());
  CHECK(vertices.size() == normals.size());
  std::vector<gl::GLuint> indices;
  for (vtkIdType id = 0; id < points->GetNumberOfPoints(); ++id) {
    points->GetPoint(id, &vertices[id][0]);
    pointsNormals->GetTuple(id, &normals[id][0]);
  }
  vtkIdType npts;
  vtkIdType* pts;
  for (int i = 0; i < polyData->GetNumberOfPolys(); ++i) {
    int h = polys->GetNextCell(npts, pts);
    if (h == 0) {
      break;
    }
    if (npts == 3) {
      indices.push_back(pts[0]);
      indices.push_back(pts[1]);
      indices.push_back(pts[2]);
    }
  }

  ZMesh msh;
  msh.setVertices(vertices);
  msh.setIndices(indices);
  msh.setNormals(normals);
  return msh;
}

vtkSmartPointer<vtkPolyData> meshToVtkPolyData(const ZMesh& mesh)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(VTK_FLOAT);
  const std::vector<glm::vec3>& vertices = mesh.vertices();
  points->Allocate(vertices.size());
  for (size_t i = 0; i < vertices.size(); ++i) {
    points->InsertPoint(i, vertices[i].x, vertices[i].y, vertices[i].z);
  }

  vtkSmartPointer<vtkFloatArray> nrmls = vtkSmartPointer<vtkFloatArray>::New();
  const std::vector<glm::vec3>& normals = mesh.normals();
  CHECK(normals.size() == vertices.size());
  nrmls->SetNumberOfComponents(3);
  nrmls->Allocate(3 * normals.size());
  nrmls->SetName("Normals");
  for (size_t i = 0; i < normals.size(); ++i) {
    nrmls->InsertTuple(i, &normals[i][0]);
  }

  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
  size_t numTriangles = mesh.numTriangles();
  polys->Allocate(numTriangles * 3);
  vtkIdType pts[3];
  for (size_t i = 0; i < numTriangles; ++i) {
    glm::uvec3 tri = mesh.triangleIndices(i);
    pts[0] = tri[0];
    pts[1] = tri[1];
    pts[2] = tri[2];
    polys->InsertNextCell(3, pts);
  }

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->GetPointData()->SetNormals(nrmls);
  polyData->SetPolys(polys);

  return polyData;
}

} // namespace

ZMesh::ZMesh(GLenum type)
{
  setType(type);
}

ZMesh::ZMesh(const QString& filename)
{
  load(filename);
}

void ZMesh::swap(ZMesh& rhs) noexcept
{
  std::swap(m_type, rhs.m_type);

  m_vertices.swap(rhs.m_vertices);
  m_1DTextureCoordinates.swap(rhs.m_1DTextureCoordinates);
  m_2DTextureCoordinates.swap(rhs.m_2DTextureCoordinates);
  m_3DTextureCoordinates.swap(rhs.m_3DTextureCoordinates);
  m_normals.swap(rhs.m_normals);
  m_colors.swap(rhs.m_colors);
  m_indices.swap(rhs.m_indices);
}

bool ZMesh::canReadFile(const QString& filename)
{
  return ZMeshIO::instance().canReadFile(filename);
}

bool ZMesh::canWriteFile(const QString& filename)
{
  return ZMeshIO::instance().canWriteFile(filename);
}

const QString& ZMesh::getQtReadNameFilter()
{
  return ZMeshIO::instance().getQtReadNameFilter();
}

void ZMesh::getQtWriteNameFilter(QStringList& filters, QList<std::string>& formats)
{
  ZMeshIO::instance().getQtWriteNameFilter(filters, formats);
}

void ZMesh::load(const QString& filename)
{
  ZMeshIO::instance().load(filename, *this);
}

void ZMesh::save(const QString& filename, const std::string& format) const
{
  ZMeshIO::instance().save(*this, filename, format);
}

ZBBox<glm::dvec3> ZMesh::boundBox() const
{
  ZBBox<glm::dvec3> result;
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    result.expand(glm::dvec3(m_vertices[i]));
  }
  return result;
}

ZBBox<glm::dvec3> ZMesh::boundBox(const glm::mat4& transform) const
{
  ZBBox<glm::dvec3> result;
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    glm::vec3 vert = glm::applyMatrix(transform, m_vertices[i]);
    result.expand(glm::dvec3(vert));
  }
  return result;
}

QString ZMesh::typeAsString() const
{
  if (m_type == GL_TRIANGLES) {
    return "GL_TRIANGLES";
  }

  if (m_type == GL_TRIANGLE_STRIP) {
    return "GL_TRIANGLE_STRIP";
  }

  if (m_type == GL_TRIANGLE_FAN) {
    return "GL_TRIANGLE_FAN";
  }

  return "WrongType";
}

std::vector<glm::dvec3> ZMesh::doubleVertices() const
{
  std::vector<glm::dvec3> result;
  for (auto v : m_vertices)
    result.emplace_back(v.x, v.y, v.z);
  return result;
}

void ZMesh::setVertices(const std::vector<glm::dvec3>& vertices)
{
  for (const auto& v : vertices) {
    m_vertices.push_back(glm::vec3(v));
  }
}

void ZMesh::setNormals(const std::vector<glm::dvec3>& normals)
{
  for (const auto& n : normals) {
    m_normals.push_back(glm::vec3(n));
  }
}

void ZMesh::interpolate(const ZMesh& ref)
{
  std::vector<glm::uvec3> triIdxs = ref.triangleIndices();
  if (!ref.m_1DTextureCoordinates.empty())
    m_1DTextureCoordinates.clear();
  if (!ref.m_2DTextureCoordinates.empty())
    m_2DTextureCoordinates.clear();
  if (!ref.m_3DTextureCoordinates.empty())
    m_3DTextureCoordinates.clear();
  if (!ref.m_colors.empty())
    m_colors.clear();
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    bool match = false;
    // first check all ref vertices
    for (size_t j = 0; !match && j < ref.m_vertices.size(); ++j) {
      if (glm::length(ref.m_vertices[j] - m_vertices[i]) <= 1e-6) {
        match = true;
        if (!ref.m_1DTextureCoordinates.empty())
          m_1DTextureCoordinates.push_back(ref.m_1DTextureCoordinates[j]);
        if (!ref.m_2DTextureCoordinates.empty())
          m_2DTextureCoordinates.push_back(ref.m_2DTextureCoordinates[j]);
        if (!ref.m_3DTextureCoordinates.empty())
          m_3DTextureCoordinates.push_back(ref.m_3DTextureCoordinates[j]);
        if (!ref.m_colors.empty())
          m_colors.push_back(ref.m_colors[j]);
      }
    }
    // no vertice match, interpolate
    for (size_t j = 0; !match && j < triIdxs.size(); ++j) {
      glm::uvec3 triIdx = triIdxs[j];
      double s, t;
      if (ZMeshUtils::vertexTriangleSquaredDistance(glm::dvec3(m_vertices[i]), glm::dvec3(ref.m_vertices[triIdx[0]]),
                                                    glm::dvec3(ref.m_vertices[triIdx[1]]),
                                                    glm::dvec3(ref.m_vertices[triIdx[2]]),
                                                    s, t)
          <= 1e-6) {
        match = true;
        float fs = static_cast<float>(s);
        float ft = static_cast<float>(t);
        if (!ref.m_1DTextureCoordinates.empty())
          m_1DTextureCoordinates.push_back(ref.m_1DTextureCoordinates[triIdx[0]] +
                                           (ref.m_1DTextureCoordinates[triIdx[1]] -
                                            ref.m_1DTextureCoordinates[triIdx[0]]) * fs +
                                           (ref.m_1DTextureCoordinates[triIdx[2]] -
                                            ref.m_1DTextureCoordinates[triIdx[0]]) * ft);
        if (!ref.m_2DTextureCoordinates.empty())
          m_2DTextureCoordinates.push_back(ref.m_2DTextureCoordinates[triIdx[0]] +
                                           (ref.m_2DTextureCoordinates[triIdx[1]] -
                                            ref.m_2DTextureCoordinates[triIdx[0]]) * fs +
                                           (ref.m_2DTextureCoordinates[triIdx[2]] -
                                            ref.m_2DTextureCoordinates[triIdx[0]]) * ft);
        if (!ref.m_3DTextureCoordinates.empty())
          m_3DTextureCoordinates.push_back(ref.m_3DTextureCoordinates[triIdx[0]] +
                                           (ref.m_3DTextureCoordinates[triIdx[1]] -
                                            ref.m_3DTextureCoordinates[triIdx[0]]) * fs +
                                           (ref.m_3DTextureCoordinates[triIdx[2]] -
                                            ref.m_3DTextureCoordinates[triIdx[0]]) * ft);
        if (!ref.m_colors.empty())
          m_colors.push_back(ref.m_colors[triIdx[0]] +
                             (ref.m_colors[triIdx[1]] - ref.m_colors[triIdx[0]]) * fs +
                             (ref.m_colors[triIdx[2]] - ref.m_colors[triIdx[0]]) * ft);
      }
    }
    if (!match) {
      if (!ref.m_1DTextureCoordinates.empty())
        m_1DTextureCoordinates.push_back(0.0);
      if (!ref.m_2DTextureCoordinates.empty())
        m_2DTextureCoordinates.emplace_back(0.0);
      if (!ref.m_3DTextureCoordinates.empty())
        m_3DTextureCoordinates.emplace_back(0.0);
      if (!ref.m_colors.empty())
        m_colors.emplace_back(0.0);
    }
  }
}

void ZMesh::clear()
{
  m_vertices.clear();
  m_1DTextureCoordinates.clear();
  m_2DTextureCoordinates.clear();
  m_3DTextureCoordinates.clear();
  m_normals.clear();
  m_colors.clear();
  m_indices.clear();
}

size_t ZMesh::numTriangles() const
{
  size_t n = 0;
  if (m_indices.empty())
    n = m_vertices.size();
  else
    n = m_indices.size();
  if (m_type == GL_TRIANGLES)
    return n / 3;
  if (m_type == GL_TRIANGLE_STRIP || m_type == GL_TRIANGLE_FAN)
    return n - 2;

  return 0;
}

std::vector<glm::vec3> ZMesh::triangleVertices(size_t index) const
{
  std::vector<glm::vec3> triangle;
  glm::uvec3 tIs = triangleIndices(index);
  triangle.push_back(m_vertices[tIs[0]]);
  triangle.push_back(m_vertices[tIs[1]]);
  triangle.push_back(m_vertices[tIs[2]]);
  return triangle;
}

std::vector<glm::uvec3> ZMesh::triangleIndices() const
{
  std::vector<glm::uvec3> result;
  if (m_indices.empty()) {
    if (m_type == GL_TRIANGLES) {
      for (size_t i = 0; i < m_vertices.size() - 2; i += 3) {
        result.emplace_back(i, i + 1, i + 2);
      }
    } else if (m_type == GL_TRIANGLE_STRIP) {
      for (size_t i = 0; i < m_vertices.size() - 2; ++i) {
        glm::uvec3 triangle;
        if (i % 2 == 0) {
          triangle[0] = i;
          triangle[1] = i + 1;
        } else {
          triangle[0] = i + 1;
          triangle[1] = i;
        }
        triangle[2] = i + 2;
        result.push_back(triangle);
      }
    } else /*(m_type == GL_TRIANGLE_FAN)*/ {
      for (size_t i = 1; i < m_vertices.size() - 1; ++i) {
        result.emplace_back(0, i, i + 1);
      }
    }
  } else {
    if (m_type == GL_TRIANGLES) {
      for (size_t i = 0; i < m_indices.size() - 2; i += 3) {
        result.emplace_back(m_indices[i], m_indices[i + 1], m_indices[i + 2]);
      }
    } else if (m_type == GL_TRIANGLE_STRIP) {
      for (size_t i = 0; i < m_indices.size() - 2; ++i) {
        glm::uvec3 triangle;
        if (i % 2 == 0) {
          triangle[0] = m_indices[i];
          triangle[1] = m_indices[i + 1];
        } else {
          triangle[0] = m_indices[i + 1];
          triangle[1] = m_indices[i];
        }
        triangle[2] = m_indices[i + 2];
        result.push_back(triangle);
      }
    } else /*(m_type == GL_TRIANGLE_FAN)*/ {
      for (size_t i = 1; i < m_indices.size() - 1; ++i) {
        result.emplace_back(m_indices[0], m_indices[i], m_indices[i + 1]);
      }
    }
  }
  return result;
}

glm::uvec3 ZMesh::triangleIndices(size_t index) const
{
  glm::uvec3 triangle;
  CHECK(index < numTriangles());
  if (m_indices.empty()) {
    if (m_type == GL_TRIANGLES) {
      triangle[0] = index * 3;
      triangle[1] = index * 3 + 1;
      triangle[2] = index * 3 + 2;
    } else if (m_type == GL_TRIANGLE_STRIP) {
      if (index % 2 == 0) {
        triangle[0] = index;
        triangle[1] = index + 1;
      } else {
        triangle[0] = index + 1;
        triangle[1] = index;
      }
      triangle[2] = index + 2;
    } else if (m_type == GL_TRIANGLE_FAN) {
      triangle[0] = 0;
      triangle[1] = index + 1;
      triangle[2] = index + 2;
    }
  } else {
    if (m_type == GL_TRIANGLES) {
      triangle[0] = m_indices[index * 3];
      triangle[1] = m_indices[index * 3 + 1];
      triangle[2] = m_indices[index * 3 + 2];
    } else if (m_type == GL_TRIANGLE_STRIP) {
      if (index % 2 == 0) {
        triangle[0] = m_indices[index];
        triangle[1] = m_indices[index + 1];
      } else {
        triangle[0] = m_indices[index + 1];
        triangle[1] = m_indices[index];
      }
      triangle[2] = m_indices[index + 2];
    } else if (m_type == GL_TRIANGLE_FAN) {
      triangle[0] = m_indices[0];
      triangle[1] = m_indices[index + 1];
      triangle[2] = m_indices[index + 2];
    }
  }
  return triangle;
}

glm::vec3 ZMesh::triangleVertex(size_t triangleIndex, size_t vertexIndex) const
{
  CHECK(vertexIndex <= 2);
  return triangleVertices(triangleIndex)[vertexIndex];
}

void ZMesh::transformVerticesByMatrix(const glm::mat4& tfmat)
{
  if (tfmat == glm::mat4(1.0))
    return;
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    m_vertices[i] = glm::applyMatrix(tfmat, m_vertices[i]);
  }
}

std::vector<ZMesh> ZMesh::split(size_t numTriangle) const
{
  size_t totalNumTri = numTriangles();
  size_t numResult = std::ceil(1.0 * totalNumTri / numTriangle);
  std::vector<ZMesh> res(numResult);
  for (size_t i = 0; i < numResult; ++i) {
    for (size_t j = i * numTriangle; j < std::min(totalNumTri, (i + 1) * numTriangle); ++j) {
      res[i].appendTriangle(*this, triangleIndices(j));
    }
  }
  return res;
}

void ZMesh::generateNormals(bool useAreaWeight)
{
  m_normals.resize(m_vertices.size());
  for (size_t i = 0; i < m_normals.size(); ++i)
    m_normals[i] = glm::vec3(0.f);

  for (size_t i = 0; i < numTriangles(); ++i) {
    glm::uvec3 tri = triangleIndices(i);
    // get the three vertices that make the faces
    glm::vec3 p1 = m_vertices[tri[0]];
    glm::vec3 p2 = m_vertices[tri[1]];
    glm::vec3 p3 = m_vertices[tri[2]];

    glm::vec3 v1 = p2 - p1;
    glm::vec3 v2 = p3 - p1;
    glm::vec3 normal = glm::cross(v1, v2);
    if (!useAreaWeight)
      normal = glm::normalize(normal);
    m_normals[tri[0]] += normal;
    m_normals[tri[1]] += normal;
    m_normals[tri[2]] += normal;
  }

  for (size_t i = 0; i < m_normals.size(); ++i)
    m_normals[i] = glm::normalize(m_normals[i]);
}

//double ZMesh::volume() const
//{
//  double res = 0;
//  for (size_t i=0; i<numTriangles(); ++i) {
//    glm::uvec3 tIs = triangleIndices(i);
//    glm::vec3 normal = glm::normalize(glm::cross(m_vertices[tIs[1]] - m_vertices[tIs[0]],
//        m_vertices[tIs[2]] - m_vertices[tIs[0]]));
//    if (glm::dot(m_normals[tIs[0]], normal) +
//        glm::dot(m_normals[tIs[1]], normal) +
//        glm::dot(m_normals[tIs[2]], normal) < 0) {
//      res += signedVolumeOfTriangle(m_vertices[tIs[0]] - m_vertices[0], m_vertices[tIs[2]] - m_vertices[0], m_vertices[tIs[1]] - m_vertices[0]);
//    } else {
//      res += signedVolumeOfTriangle(m_vertices[tIs[0]] - m_vertices[0], m_vertices[tIs[1]] - m_vertices[0], m_vertices[tIs[2]] - m_vertices[0]);
//    }
//  }
//  return std::abs(res);
//}

ZMeshProperties ZMesh::properties() const
{
  vtkSmartPointer<vtkPolyData> poly = meshToVtkPolyData(*this);
  vtkSmartPointer<vtkMassProperties> massProperties = vtkSmartPointer<vtkMassProperties>::New();
  massProperties->SetInputData(poly);

  massProperties->Update();

  ZMeshProperties res;
  res.numVertices = numVertices();
  res.numTriangles = numTriangles();
  res.kx = massProperties->GetKx();
  res.ky = massProperties->GetKy();
  res.kz = massProperties->GetKz();
  res.maxTriangleArea = massProperties->GetMaxCellArea();
  res.minTriangleArea = massProperties->GetMinCellArea();
  res.normalizedShapeIndex = massProperties->GetNormalizedShapeIndex();
  res.surfaceArea = massProperties->GetSurfaceArea();
  res.volume = massProperties->GetVolume();
  res.volumeProjected = massProperties->GetVolumeProjected();
  res.volumeX = massProperties->GetVolumeX();
  res.volumeY = massProperties->GetVolumeY();
  res.volumeZ = massProperties->GetVolumeZ();
  return res;
}

void ZMesh::logProperties(const ZMeshProperties& prop, const QString& str)
{
  LOG(INFO) << "";
  if (!str.isEmpty()) {
    LOG(INFO) << str;
  }
  LOG(INFO) << "Vertices Number: " << prop.numVertices;
  LOG(INFO) << "Triangles Number: " << prop.numTriangles;
  //LOG(INFO) << "volume old: " << volume();
  LOG(INFO) << "Surface Area: " << prop.surfaceArea;
  LOG(INFO) << "Min Triangle Area: " << prop.minTriangleArea;
  LOG(INFO) << "Max Triangle Area: " << prop.maxTriangleArea;
  LOG(INFO) << "Volume: " << prop.volume;
  LOG(INFO) << "Volume Projected: " << prop.volumeProjected;
  LOG(INFO) << "Volume X: " << prop.volumeX;
  LOG(INFO) << "Volume Y: " << prop.volumeY;
  LOG(INFO) << "Volume Z: " << prop.volumeZ;
  LOG(INFO) << "Kx: " << prop.kx;
  LOG(INFO) << "Ky: " << prop.ky;
  LOG(INFO) << "Kz: " << prop.kz;
  LOG(INFO) << "Normalized Shape Index: " << prop.normalizedShapeIndex;
  LOG(INFO) << "";
}

ZMesh ZMesh::createCubesWithNormal(const std::vector<glm::vec3>& coordLlfs, const std::vector<glm::vec3>& coordUrbs)
{
  CHECK(coordLlfs.size() == coordUrbs.size());
  ZMesh cubes(GL_TRIANGLES);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<GLuint> indexes;
  GLuint idxes[6] = {0, 1, 2, 2, 1, 3};

  for (size_t i = 0; i < coordLlfs.size(); ++i) {
    //CHECK(coordUrbs[i].z > coordLlfs[i].z && coordUrbs[i].y > coordLlfs[i].y && coordUrbs[i].x > coordLlfs[i].x);
    glm::vec3 p0(coordLlfs[i][0], coordLlfs[i][1], coordUrbs[i][2]);
    glm::vec3 p1(coordUrbs[i][0], coordLlfs[i][1], coordUrbs[i][2]);
    glm::vec3 p2(coordLlfs[i][0], coordUrbs[i][1], coordUrbs[i][2]);
    glm::vec3 p3(coordUrbs[i][0], coordUrbs[i][1], coordUrbs[i][2]);
    glm::vec3 p4(coordLlfs[i][0], coordLlfs[i][1], coordLlfs[i][2]);
    glm::vec3 p5(coordUrbs[i][0], coordLlfs[i][1], coordLlfs[i][2]);
    glm::vec3 p6(coordLlfs[i][0], coordUrbs[i][1], coordLlfs[i][2]);
    glm::vec3 p7(coordUrbs[i][0], coordUrbs[i][1], coordLlfs[i][2]);
    glm::vec3 frontFaceNormal = glm::normalize(p4 - p0);
    glm::vec3 rightFaceNormal = glm::normalize(p5 - p4);
    glm::vec3 upFaceNormal = glm::normalize(p4 - p6);

    vertices.push_back(p0);
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);

    normals.push_back(-frontFaceNormal);
    normals.push_back(-frontFaceNormal);
    normals.push_back(-frontFaceNormal);
    normals.push_back(-frontFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 0 + i * 24);
    }

    vertices.push_back(p2);
    vertices.push_back(p3);
    vertices.push_back(p6);
    vertices.push_back(p7);

    normals.push_back(-upFaceNormal);
    normals.push_back(-upFaceNormal);
    normals.push_back(-upFaceNormal);
    normals.push_back(-upFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 4 + i * 24);
    }

    vertices.push_back(p4);
    vertices.push_back(p0);
    vertices.push_back(p6);
    vertices.push_back(p2);

    normals.push_back(-rightFaceNormal);
    normals.push_back(-rightFaceNormal);
    normals.push_back(-rightFaceNormal);
    normals.push_back(-rightFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 2 * 4 + i * 24);
    }

    vertices.push_back(p7);
    vertices.push_back(p3);
    vertices.push_back(p5);
    vertices.push_back(p1);

    normals.push_back(rightFaceNormal);
    normals.push_back(rightFaceNormal);
    normals.push_back(rightFaceNormal);
    normals.push_back(rightFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 3 * 4 + i * 24);
    }

    vertices.push_back(p4);
    vertices.push_back(p5);
    vertices.push_back(p0);
    vertices.push_back(p1);

    normals.push_back(upFaceNormal);
    normals.push_back(upFaceNormal);
    normals.push_back(upFaceNormal);
    normals.push_back(upFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 4 * 4 + i * 24);
    }

    vertices.push_back(p6);
    vertices.push_back(p7);
    vertices.push_back(p4);
    vertices.push_back(p5);

    normals.push_back(frontFaceNormal);
    normals.push_back(frontFaceNormal);
    normals.push_back(frontFaceNormal);
    normals.push_back(frontFaceNormal);

    for (GLuint j = 0; j < 6; ++j) {
      indexes.push_back(idxes[j] + 5 * 4 + i * 24);
    }
  }

  cubes.m_vertices.swap(vertices);
  cubes.m_normals.swap(normals);
  cubes.m_indices.swap(indexes);
  return cubes;
}

ZMesh ZMesh::createCube(const glm::vec3& coordLlf, const glm::vec3& coordUrb,
                        const glm::vec3& texLlf, const glm::vec3& texUrb)
{
  ZMesh cube(GL_TRIANGLE_STRIP);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> texCoords;
  vertices.emplace_back(coordLlf[0], coordLlf[1], coordUrb[2]);
  vertices.emplace_back(coordUrb[0], coordLlf[1], coordUrb[2]);
  vertices.emplace_back(coordLlf[0], coordUrb[1], coordUrb[2]);
  vertices.emplace_back(coordUrb[0], coordUrb[1], coordUrb[2]);
  vertices.emplace_back(coordLlf[0], coordLlf[1], coordLlf[2]);
  vertices.emplace_back(coordUrb[0], coordLlf[1], coordLlf[2]);
  vertices.emplace_back(coordLlf[0], coordUrb[1], coordLlf[2]);
  vertices.emplace_back(coordUrb[0], coordUrb[1], coordLlf[2]);

  texCoords.emplace_back(texLlf[0], texLlf[1], texUrb[2]);
  texCoords.emplace_back(texUrb[0], texLlf[1], texUrb[2]);
  texCoords.emplace_back(texLlf[0], texUrb[1], texUrb[2]);
  texCoords.emplace_back(texUrb[0], texUrb[1], texUrb[2]);
  texCoords.emplace_back(texLlf[0], texLlf[1], texLlf[2]);
  texCoords.emplace_back(texUrb[0], texLlf[1], texLlf[2]);
  texCoords.emplace_back(texLlf[0], texUrb[1], texLlf[2]);
  texCoords.emplace_back(texUrb[0], texUrb[1], texLlf[2]);

  GLuint idxes[14] = {0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1};
  std::vector<GLuint> indexes(idxes, idxes + 14);
  cube.m_vertices.swap(vertices);
  cube.m_3DTextureCoordinates.swap(texCoords);
  cube.m_indices.swap(indexes);
  return cube;
}

ZMesh ZMesh::createCubeSlice(float coordIn3rdDim, float texCoordIn3rdDim, int alongDim,
                             const glm::vec2& coordlow, const glm::vec2& coordhigh,
                             const glm::vec2& texlow, const glm::vec2& texhigh)
{
  ZMesh quad(GL_TRIANGLE_STRIP);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> texCoords;
  if (alongDim == 0) {
    vertices.emplace_back(coordIn3rdDim, coordlow[0], coordlow[1]);
    vertices.emplace_back(coordIn3rdDim, coordhigh[0], coordlow[1]);
    vertices.emplace_back(coordIn3rdDim, coordlow[0], coordhigh[1]);
    vertices.emplace_back(coordIn3rdDim, coordhigh[0], coordhigh[1]);
    texCoords.emplace_back(texCoordIn3rdDim, texlow[0], texlow[1]);
    texCoords.emplace_back(texCoordIn3rdDim, texhigh[0], texlow[1]);
    texCoords.emplace_back(texCoordIn3rdDim, texlow[0], texhigh[1]);
    texCoords.emplace_back(texCoordIn3rdDim, texhigh[0], texhigh[1]);
  } else if (alongDim == 1) {
    vertices.emplace_back(coordlow[0], coordIn3rdDim, coordlow[1]);
    vertices.emplace_back(coordhigh[0], coordIn3rdDim, coordlow[1]);
    vertices.emplace_back(coordlow[0], coordIn3rdDim, coordhigh[1]);
    vertices.emplace_back(coordhigh[0], coordIn3rdDim, coordhigh[1]);
    texCoords.emplace_back(texlow[0], texCoordIn3rdDim, texlow[1]);
    texCoords.emplace_back(texhigh[0], texCoordIn3rdDim, texlow[1]);
    texCoords.emplace_back(texlow[0], texCoordIn3rdDim, texhigh[1]);
    texCoords.emplace_back(texhigh[0], texCoordIn3rdDim, texhigh[1]);
  } else if (alongDim == 2) {
    vertices.emplace_back(coordlow[0], coordlow[1], coordIn3rdDim);
    vertices.emplace_back(coordhigh[0], coordlow[1], coordIn3rdDim);
    vertices.emplace_back(coordlow[0], coordhigh[1], coordIn3rdDim);
    vertices.emplace_back(coordhigh[0], coordhigh[1], coordIn3rdDim);
    texCoords.emplace_back(texlow[0], texlow[1], texCoordIn3rdDim);
    texCoords.emplace_back(texhigh[0], texlow[1], texCoordIn3rdDim);
    texCoords.emplace_back(texlow[0], texhigh[1], texCoordIn3rdDim);
    texCoords.emplace_back(texhigh[0], texhigh[1], texCoordIn3rdDim);
  }

  quad.m_vertices.swap(vertices);
  quad.m_3DTextureCoordinates.swap(texCoords);
  return quad;
}

ZMesh ZMesh::createCubeSliceWith2DTexture(float coordIn3rdDim, int alongDim,
                                          const glm::vec2& coordlow, const glm::vec2& coordhigh,
                                          const glm::vec2& texlow, const glm::vec2& texhigh)
{
  ZMesh quad(GL_TRIANGLE_STRIP);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> texCoords;
  if (alongDim == 0) {
    vertices.emplace_back(coordIn3rdDim, coordlow[0], coordlow[1]);
    vertices.emplace_back(coordIn3rdDim, coordhigh[0], coordlow[1]);
    vertices.emplace_back(coordIn3rdDim, coordlow[0], coordhigh[1]);
    vertices.emplace_back(coordIn3rdDim, coordhigh[0], coordhigh[1]);
  } else if (alongDim == 1) {
    vertices.emplace_back(coordlow[0], coordIn3rdDim, coordlow[1]);
    vertices.emplace_back(coordhigh[0], coordIn3rdDim, coordlow[1]);
    vertices.emplace_back(coordlow[0], coordIn3rdDim, coordhigh[1]);
    vertices.emplace_back(coordhigh[0], coordIn3rdDim, coordhigh[1]);
  } else if (alongDim == 2) {
    vertices.emplace_back(coordlow[0], coordlow[1], coordIn3rdDim);
    vertices.emplace_back(coordhigh[0], coordlow[1], coordIn3rdDim);
    vertices.emplace_back(coordlow[0], coordhigh[1], coordIn3rdDim);
    vertices.emplace_back(coordhigh[0], coordhigh[1], coordIn3rdDim);
  }
  texCoords.emplace_back(texlow[0], texlow[1]);
  texCoords.emplace_back(texhigh[0], texlow[1]);
  texCoords.emplace_back(texlow[0], texhigh[1]);
  texCoords.emplace_back(texhigh[0], texhigh[1]);

  quad.m_vertices.swap(vertices);
  quad.m_2DTextureCoordinates.swap(texCoords);
  return quad;
}

ZMesh ZMesh::createImageSlice(float coordIn3rdDim, const glm::vec2& coordlow,
                              const glm::vec2& coordhigh, const glm::vec2& texlow, const glm::vec2& texhigh)
{
  ZMesh quad(GL_TRIANGLE_STRIP);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> texCoords;

  vertices.emplace_back(coordlow[0], coordlow[1], coordIn3rdDim);
  vertices.emplace_back(coordhigh[0], coordlow[1], coordIn3rdDim);
  vertices.emplace_back(coordlow[0], coordhigh[1], coordIn3rdDim);
  vertices.emplace_back(coordhigh[0], coordhigh[1], coordIn3rdDim);
  texCoords.emplace_back(texlow[0], texlow[1]);
  texCoords.emplace_back(texhigh[0], texlow[1]);
  texCoords.emplace_back(texlow[0], texhigh[1]);
  texCoords.emplace_back(texhigh[0], texhigh[1]);

  quad.m_vertices.swap(vertices);
  quad.m_2DTextureCoordinates.swap(texCoords);
  return quad;
}

ZMesh ZMesh::createCubeSerieSlices(int numSlices, int alongDim, const glm::vec3& coordfirst,
                                   const glm::vec3& coordlast, const glm::vec3& texfirst, const glm::vec3& texlast)
{
  ZMesh quad(GL_TRIANGLES);
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> texCoords;
  std::vector<GLuint> indexes;
  GLuint idx[6] = {0, 1, 2, 2, 1, 3};

  bool reverse = true;
  if (alongDim == 0 && coordfirst.x > coordlast.x)
    reverse = false;
  if (alongDim == 1 && coordfirst.y > coordlast.y)
    reverse = false;
  if (alongDim == 2 && coordfirst.z > coordlast.z)
    reverse = false;

  for (int i = 0; i < numSlices; ++i) {
    float factor = 0.f;
    if (numSlices > 1)
      factor = i / (numSlices - 1.0);
    if (alongDim == 0) {
      vertices.emplace_back(glm::mix(coordfirst.x, coordlast.x, factor), coordfirst[1], coordfirst[2]);
      vertices.emplace_back(glm::mix(coordfirst.x, coordlast.x, factor), coordlast[1], coordfirst[2]);
      vertices.emplace_back(glm::mix(coordfirst.x, coordlast.x, factor), coordfirst[1], coordlast[2]);
      vertices.emplace_back(glm::mix(coordfirst.x, coordlast.x, factor), coordlast[1], coordlast[2]);
      texCoords.emplace_back(glm::mix(texfirst.x, texlast.x, factor), texfirst[1], texfirst[2]);
      texCoords.emplace_back(glm::mix(texfirst.x, texlast.x, factor), texlast[1], texfirst[2]);
      texCoords.emplace_back(glm::mix(texfirst.x, texlast.x, factor), texfirst[1], texlast[2]);
      texCoords.emplace_back(glm::mix(texfirst.x, texlast.x, factor), texlast[1], texlast[2]);
    } else if (alongDim == 1) {
      vertices.emplace_back(coordfirst[0], glm::mix(coordfirst.y, coordlast.y, factor), coordfirst[2]);
      vertices.emplace_back(coordlast[0], glm::mix(coordfirst.y, coordlast.y, factor), coordfirst[2]);
      vertices.emplace_back(coordfirst[0], glm::mix(coordfirst.y, coordlast.y, factor), coordlast[2]);
      vertices.emplace_back(coordlast[0], glm::mix(coordfirst.y, coordlast.y, factor), coordlast[2]);
      texCoords.emplace_back(texfirst[0], glm::mix(texfirst.y, texlast.y, factor), texfirst[2]);
      texCoords.emplace_back(texlast[0], glm::mix(texfirst.y, texlast.y, factor), texfirst[2]);
      texCoords.emplace_back(texfirst[0], glm::mix(texfirst.y, texlast.y, factor), texlast[2]);
      texCoords.emplace_back(texlast[0], glm::mix(texfirst.y, texlast.y, factor), texlast[2]);
    } else {
      vertices.emplace_back(coordfirst[0], coordfirst[1], glm::mix(coordfirst.z, coordlast.z, factor));
      vertices.emplace_back(coordlast[0], coordfirst[1], glm::mix(coordfirst.z, coordlast.z, factor));
      vertices.emplace_back(coordfirst[0], coordlast[1], glm::mix(coordfirst.z, coordlast.z, factor));
      vertices.emplace_back(coordlast[0], coordlast[1], glm::mix(coordfirst.z, coordlast.z, factor));
      texCoords.emplace_back(texfirst[0], texfirst[1], glm::mix(texfirst.z, texlast.z, factor));
      texCoords.emplace_back(texlast[0], texfirst[1], glm::mix(texfirst.z, texlast.z, factor));
      texCoords.emplace_back(texfirst[0], texlast[1], glm::mix(texfirst.z, texlast.z, factor));
      texCoords.emplace_back(texlast[0], texlast[1], glm::mix(texfirst.z, texlast.z, factor));
    }
    for (int j = 0; j < 6; ++j) {
      if (reverse)
        indexes.push_back(idx[5 - j] + i * 4);
      else
        indexes.push_back(idx[j] + i * 4);
    }
  }

  quad.m_vertices.swap(vertices);
  quad.m_3DTextureCoordinates.swap(texCoords);
  quad.m_indices.swap(indexes);
  return quad;
}

ZMesh ZMesh::createSphereMesh(const glm::vec3& center, float radius,
                              int thetaResolution, int phiResolution,
                              float startTheta, float endTheta,
                              float startPhi, float endPhi)
{
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();

  sphereSource->SetCenter(center.x, center.y, center.z);
  sphereSource->SetRadius(radius);
  sphereSource->SetThetaResolution(thetaResolution);
  sphereSource->SetPhiResolution(phiResolution);
  sphereSource->SetStartTheta(startTheta);
  sphereSource->SetEndTheta(endTheta);
  sphereSource->SetStartPhi(startPhi);
  sphereSource->SetEndPhi(endPhi);
  sphereSource->SetLatLongTessellation(false);
  sphereSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  sphereSource->Update();
  return vtkPolyDataToMesh(sphereSource->GetOutput());
}

ZMesh ZMesh::createTubeMesh(const std::vector<glm::vec3>& line, const std::vector<float>& radius,
                            int numberOfSides, bool capping)
{
  CHECK(line.size() == radius.size());
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(VTK_FLOAT);
  for (size_t i = 0; i < line.size(); ++i) {
    points->InsertPoint(i, line[i].x, line[i].y, line[i].z);
  }

  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(line.size());
  for (size_t i = 0; i < line.size(); ++i) {
    lines->InsertCellPoint(i);
  }

  vtkSmartPointer<vtkFloatArray> tubeRadius = vtkSmartPointer<vtkFloatArray>::New();
  tubeRadius->SetName("TubeRadius");
  tubeRadius->SetNumberOfTuples(radius.size());
  for (size_t i = 0; i < radius.size(); ++i) {
    tubeRadius->SetTuple1(i, radius[i]);
  }

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetLines(lines);
  polyData->GetPointData()->AddArray(tubeRadius);
  polyData->GetPointData()->SetActiveScalars("TubeRadius");

  vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();

  tubeFilter->SetInputData(polyData);
  tubeFilter->SetNumberOfSides(numberOfSides);
  tubeFilter->SetCapping(capping);
  tubeFilter->SetSidesShareVertices(true);
  tubeFilter->SetVaryRadiusToVaryRadiusByAbsoluteScalar();

  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(tubeFilter->GetOutputPort());

  triangleFilter->Update();
  return vtkPolyDataToMesh(triangleFilter->GetOutput());
}

ZMesh ZMesh::createConeMesh(glm::vec3 base, float baseRadius, glm::vec3 top, float topRadius,
                            int numberOfSides, bool capping)
{
  CHECK(baseRadius >= 0 && topRadius >= 0 && numberOfSides > 2);
  if (baseRadius > topRadius) {
    std::swap(base, top);
    std::swap(baseRadius, topRadius);
  }
  glm::vec3 axis = glm::normalize(top - base);
  // vector perpendicular to axis
  glm::vec3 v1, v2;
  glm::getOrthogonalVectors(axis, v1, v2);

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<GLuint> triangles;

  using namespace boost::math::float_constants;
  float theta = two_pi / numberOfSides;

  if (baseRadius == 0) {
    vertices.push_back(base);
    normals.push_back(-axis);
    for (int i = 0; i < numberOfSides; ++i) {
      glm::vec3 normal = v1 * std::cos(i * theta) + v2 * std::sin(i * theta);
      glm::vec3 tpt = top + topRadius * normal;
      glm::vec3 edge = tpt - base;
      normal = glm::normalize(glm::cross(edge, glm::cross(tpt - top, edge)));
      vertices.push_back(tpt);
      normals.push_back(normal);
    }

    size_t numVertices = vertices.size();
    for (size_t i = 1; i < numVertices; ++i) {
      triangles.push_back(0);
      triangles.push_back(i);
      triangles.push_back((i + 1) < numVertices ? (i + 1) : 1);
    }

    if (capping) {
      for (int i = 0; i < numberOfSides; ++i) {
        glm::vec3 normal = v1 * std::cos(i * theta) + v2 * std::sin(i * theta);
        glm::vec3 tpt = top + topRadius * normal;
        vertices.push_back(tpt);
        normals.push_back(axis);
      }
      // top cap
      for (size_t i = numVertices + 1; i < numVertices + numberOfSides - 1; ++i) {
        triangles.push_back(numVertices);
        triangles.push_back(i);
        triangles.push_back(i + 1);
      }
    }
  } else {
    for (int i = 0; i < numberOfSides; ++i) {
      glm::vec3 normal = v1 * std::cos(i * theta) + v2 * std::sin(i * theta);
      glm::vec3 bpt = base + baseRadius * normal;
      glm::vec3 tpt = top + topRadius * normal;
      if (topRadius != baseRadius) {
        glm::vec3 edge = tpt - bpt;
        normal = glm::normalize(glm::cross(edge, glm::cross(tpt - top, edge)));
      }
      vertices.push_back(tpt);
      normals.push_back(normal);
      vertices.push_back(bpt);
      normals.push_back(normal);
    }

    size_t numVertices = vertices.size();
    for (size_t i = 0; i < numVertices; i += 2) {
      triangles.push_back(i);
      triangles.push_back(i + 1);
      triangles.push_back((i + 3) % numVertices);

      triangles.push_back(i);
      triangles.push_back((i + 3) % numVertices);
      triangles.push_back((i + 2) % numVertices);
    }

    if (capping) {
      for (int i = 0; i < numberOfSides; ++i) {
        glm::vec3 normal = v1 * std::cos(i * theta) + v2 * std::sin(i * theta);
        glm::vec3 tpt = top + topRadius * normal;
        vertices.push_back(tpt);
        normals.push_back(axis);
      }
      for (int i = 0; i < numberOfSides; ++i) {
        glm::vec3 normal = v1 * std::cos(i * theta) + v2 * std::sin(i * theta);
        glm::vec3 bpt = base + baseRadius * normal;
        vertices.push_back(bpt);
        normals.push_back(-axis);
      }
      // top cap
      for (size_t i = numVertices + 1; i < numVertices + numberOfSides - 1; ++i) {
        triangles.push_back(numVertices);
        triangles.push_back(i);
        triangles.push_back(i + 1);
      }
      // bot cap
      numVertices += numberOfSides;
      for (size_t i = numVertices + 1; i < numVertices + numberOfSides - 1; ++i) {
        triangles.push_back(numVertices);
        triangles.push_back(i);
        triangles.push_back(i + 1);
      }
    }
  }

  ZMesh msh;
  msh.setVertices(vertices);
  msh.setIndices(triangles);
  msh.setNormals(normals);
  return msh;
}

ZMesh ZMesh::merge(const std::vector<ZMesh>& meshes)
{
  ZMesh res;
  if (meshes.empty())
    return res;
  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
  std::vector<vtkSmartPointer<vtkPolyData>> polys(meshes.size());
  for (size_t i = 0; i < meshes.size(); ++i) {
    polys[i] = meshToVtkPolyData(meshes[i]);
    appendFilter->AddInputData(polys[i]);
  }

  vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(appendFilter->GetOutputPort());

  cleanFilter->Update();
  return vtkPolyDataToMesh(cleanFilter->GetOutput());
}

void ZMesh::appendTriangle(const ZMesh& mesh, const glm::uvec3& triangle)
{
  if (!m_indices.empty() || m_type != GL_TRIANGLES)
    return;

  m_vertices.push_back(mesh.m_vertices[triangle[0]]);
  m_vertices.push_back(mesh.m_vertices[triangle[1]]);
  m_vertices.push_back(mesh.m_vertices[triangle[2]]);

  if (mesh.num1DTextureCoordinates() > 0) {
    m_1DTextureCoordinates.push_back(mesh.m_1DTextureCoordinates[triangle[0]]);
    m_1DTextureCoordinates.push_back(mesh.m_1DTextureCoordinates[triangle[1]]);
    m_1DTextureCoordinates.push_back(mesh.m_1DTextureCoordinates[triangle[2]]);
  }

  if (mesh.num2DTextureCoordinates() > 0) {
    m_2DTextureCoordinates.push_back(mesh.m_2DTextureCoordinates[triangle[0]]);
    m_2DTextureCoordinates.push_back(mesh.m_2DTextureCoordinates[triangle[1]]);
    m_2DTextureCoordinates.push_back(mesh.m_2DTextureCoordinates[triangle[2]]);
  }

  if (mesh.num3DTextureCoordinates() > 0) {
    m_3DTextureCoordinates.push_back(mesh.m_3DTextureCoordinates[triangle[0]]);
    m_3DTextureCoordinates.push_back(mesh.m_3DTextureCoordinates[triangle[1]]);
    m_3DTextureCoordinates.push_back(mesh.m_3DTextureCoordinates[triangle[2]]);
  }

  if (mesh.numNormals() > 0) {
    m_normals.push_back(mesh.m_normals[triangle[0]]);
    m_normals.push_back(mesh.m_normals[triangle[1]]);
    m_normals.push_back(mesh.m_normals[triangle[2]]);
  }

  if (mesh.numColors() > 0) {
    m_colors.push_back(mesh.m_colors[triangle[0]]);
    m_colors.push_back(mesh.m_colors[triangle[1]]);
    m_colors.push_back(mesh.m_colors[triangle[2]]);
  }
}

double ZMesh::signedVolumeOfTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) const
{
#if 1
  double v321 = v3.x * v2.y * v1.z;
  double v231 = v2.x * v3.y * v1.z;
  double v312 = v3.x * v1.y * v2.z;
  double v132 = v1.x * v3.y * v2.z;
  double v213 = v2.x * v1.y * v3.z;
  double v123 = v1.x * v2.y * v3.z;
  return (1.0 / 6.0) * (-v321 + v231 + v312 - v132 - v213 + v123);
#else
  return glm::dot(glm::dvec3(v1), glm::cross(glm::dvec3(v2), glm::dvec3(v3))) / 6.0;
#endif
}

size_t ZMesh::numCoverCubes(double cubeEdgeLength)
{
  float minx = std::numeric_limits<float>::max();
  float maxx = std::numeric_limits<float>::lowest();
  float miny = minx;
  float maxy = maxx;
  float minz = minx;
  float maxz = maxx;
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    minx = std::min(minx, m_vertices[i].x);
    maxx = std::max(maxx, m_vertices[i].x);
    miny = std::min(miny, m_vertices[i].y);
    maxy = std::max(maxy, m_vertices[i].y);
    minz = std::min(minz, m_vertices[i].z);
    maxz = std::max(maxz, m_vertices[i].z);
  }
  int xdim = std::ceil((maxx - minx) / cubeEdgeLength);
  int ydim = std::ceil((maxy - miny) / cubeEdgeLength);
  int zdim = std::ceil((maxz - minz) / cubeEdgeLength);
  std::vector<ZBBox<glm::vec3>> boxes;
  std::vector<int> numPts;
  for (int x = 0; x < xdim; ++x) {
    for (int y = 0; y < ydim; ++y) {
      for (int z = 0; z < zdim; ++z) {
        glm::vec3 minCoord(minx + x * cubeEdgeLength,
                           miny + y * cubeEdgeLength,
                           minz + z * cubeEdgeLength);
        glm::vec3 maxCoord = minCoord + glm::vec3(cubeEdgeLength, cubeEdgeLength, cubeEdgeLength);
        ZBBox<glm::vec3> box(minCoord, maxCoord);
        boxes.push_back(box);
        numPts.push_back(0);
      }
    }
  }
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    for (size_t j = 0; j < boxes.size(); ++j) {
      if (boxes[j].contains(m_vertices[i])) {
        numPts[j] += 1;
        break;
      }
    }
  }
  size_t res = 0;
  for (size_t j = 0; j < boxes.size(); ++j) {
    if (numPts[j] > 0)
      ++res;
  }
  return res;
}

ZMesh ZMesh::booleanOperation(const ZMesh& mesh1, const ZMesh& mesh2, ZMesh::BooleanOperationType type)
{
  vtkSmartPointer<vtkPolyData> input1 = meshToVtkPolyData(mesh1);
  vtkSmartPointer<vtkPolyData> input2 = meshToVtkPolyData(mesh2);

  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperationFilter =
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();

  booleanOperationFilter->SetInputData(0, input1);
  booleanOperationFilter->SetInputData(1, input2);
  switch (type) {
    case BooleanOperationType::Union:
      booleanOperationFilter->SetOperationToUnion();
      break;
    case BooleanOperationType::Intersection:
      booleanOperationFilter->SetOperationToIntersection();
      break;
    case BooleanOperationType::Difference:
      booleanOperationFilter->SetOperationToDifference();
      break;
    default:
      break;
  }
  booleanOperationFilter->SetReorientDifferenceCells(1);
  booleanOperationFilter->SetTolerance(1e-6);

  vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(booleanOperationFilter->GetOutputPort());

  cleanFilter->Update();
  return vtkPolyDataToMesh(cleanFilter->GetOutput());
}

ZSTACKOBJECT_DEFINE_CLASS_NAME(ZMesh)
