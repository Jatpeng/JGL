#include "pch.h"
#include "import_obj.h"
#include "elems/vertex_holder.h"
#include <map>
#include <cstdint>

namespace nmesh_import
{
  bool ObjMeshImporter::from_file(const std::string& filepath, nelems::Mesh* pMesh)
  {
    std::ifstream in(filepath, std::ios::in);
    if (!in)
    {
      return false;
    }

    pMesh->clear();

    std::vector<glm::vec3> t_vert;
    std::vector<glm::vec2> t_tex;
    std::vector<glm::vec3> t_norm;
    std::map<std::string, uint32_t> vertex_cache;

    auto parse_vertex = [&](const std::string& token) -> uint32_t {
      if (vertex_cache.count(token)) {
        return vertex_cache[token];
      }

      // Parse token: v/vt/vn
      std::vector<std::string> parts;
      std::stringstream ss(token);
      std::string part;
      while (std::getline(ss, part, '/')) {
        parts.push_back(part);
      }
      // Handle trailing slashes or empty parts if necessary,
      // but OBJ usually has at most 3 parts.
      // If token is "1//3", parts will be ["1", "", "3"]

      uint32_t v_idx = 0;
      uint32_t vt_idx = 0;
      uint32_t vn_idx = 0;

      if (parts.size() >= 1 && !parts[0].empty()) {
        v_idx = std::stoi(parts[0]);
      }
      if (parts.size() >= 2 && !parts[1].empty()) {
        vt_idx = std::stoi(parts[1]);
      }
      if (parts.size() >= 3 && !parts[2].empty()) {
        vn_idx = std::stoi(parts[2]);
      }

      nelems::VertexHolder vh;
      if (v_idx > 0 && v_idx <= t_vert.size()) {
        vh.mPos = t_vert[v_idx - 1];
      }
      if (vt_idx > 0 && vt_idx <= t_tex.size()) {
        vh.mTextureCoords = t_tex[vt_idx - 1];
      }
      if (vn_idx > 0 && vn_idx <= t_norm.size()) {
        vh.mNormal = t_norm[vn_idx - 1];
      }

      uint32_t new_idx = static_cast<uint32_t>(vertex_cache.size());
      pMesh->add_vertex(vh);
      vertex_cache[token] = new_idx;
      return new_idx;
    };

    std::string s_line;
    while (std::getline(in, s_line))
    {
      if (s_line.empty() || s_line[0] == '#') continue;

      std::istringstream ss_line(s_line);
      std::string id;
      ss_line >> id;

      if (id == "v")
      {
        glm::vec3 v;
        ss_line >> v.x >> v.y >> v.z;
        t_vert.push_back(v);
      }
      else if (id == "vt")
      {
        glm::vec2 vt;
        ss_line >> vt.x >> vt.y;
        t_tex.push_back(vt);
      }
      else if (id == "vn")
      {
        glm::vec3 vn;
        ss_line >> vn.x >> vn.y >> vn.z;
        t_norm.push_back(vn);
      }
      else if (id == "f")
      {
        std::vector<std::string> tokens;
        std::string token;
        while (ss_line >> token) {
          tokens.push_back(token);
        }

        if (tokens.size() >= 3) {
          uint32_t first_idx = parse_vertex(tokens[0]);
          uint32_t prev_idx = parse_vertex(tokens[1]);

          for (size_t i = 2; i < tokens.size(); ++i) {
            uint32_t curr_idx = parse_vertex(tokens[i]);
            pMesh->add_vertex_index(first_idx);
            pMesh->add_vertex_index(prev_idx);
            pMesh->add_vertex_index(curr_idx);
            prev_idx = curr_idx;
          }
        }
      }
    }

    pMesh->create_buffers();
    return true;
  }
}
