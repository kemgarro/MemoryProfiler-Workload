#include "../include/Serializer.hpp"
#include <string>
#include <cstdint>   // uint64_t, uintptr_t

namespace mp {

  // Convierte un entero sin signo de 64 bits a string
  static inline std::string u64_to_str(uint64_t v){
    return std::to_string((unsigned long long)v);
  }

  // Convierte un puntero a string representando su direccion numerica
  static inline std::string ptr_to_str(const void* p){
    return std::to_string(reinterpret_cast<std::uintptr_t>(p));
  }

  // Escapa caracteres especiales para que un string sea valido en JSON
  static inline std::string json_escape(const std::string& s){
    std::string out;
    out.reserve(s.size()+8);
    for (char c : s) {
      if (c=='\\' || c=='\"') {
        out.push_back('\\');
        out.push_back(c);
      }
      else if (c=='\n') out += "\\n";
      else out.push_back(c);
    }
    return out;
  }

  // Genera un JSON con las metricas generales de memoria
  std::string make_summary_json(std::size_t b, std::size_t p, std::size_t c){
    std::string j = "{\"bytes_in_use\":" + std::to_string(b) +
                    ",\"peak\":"        + std::to_string(p) +
                    ",\"alloc_count\":" + std::to_string(c) + "}";
    return j;
  }

  // Genera un CSV con la lista de bloques de memoria vivos
  std::string make_live_allocs_csv(const std::vector<BlockInfo>& v){
    std::string out = "ptr,size,alloc_id,thread_id,t_ns,callsite\n";
    out.reserve(out.size()+v.size()*64);
    for (const auto& b : v){
      out += ptr_to_str(b.ptr); out += ",";
      out += std::to_string(b.size); out += ",";
      out += u64_to_str(b.alloc_id); out += ",";
      out += std::to_string(b.thread_id); out += ",";
      out += u64_to_str(b.t_ns); out += ",";
      out += b.callsite; out += "\n";
    }
    return out;
  }

  // Genera un JSON con la lista de bloques de memoria vivos
  std::string make_live_allocs_json(const std::vector<BlockInfo>& v){
    std::string j = "{\"blocks\":[";
    bool first=true;
    for (const auto& b : v){
      if(!first) j += ",";
      first=false;
      j += "{\"ptr\":\""+ptr_to_str(b.ptr)+"\",";
      j += "\"size\":"+std::to_string(b.size)+",";
      j += "\"alloc_id\":"+u64_to_str(b.alloc_id)+",";
      j += "\"thread_id\":"+std::to_string(b.thread_id)+",";
      j += "\"t_ns\":"+u64_to_str(b.t_ns)+",";
      j += "\"callsite\":\""+json_escape(b.callsite)+"\",";
      j += "\"file\":\""+json_escape(b.file)+"\",";
      j += "\"line\":"+std::to_string(b.line)+",";
      j += "\"type_name\":\""+json_escape(b.type_name)+"\"}";
    }
    j += "]}";
    return j;
  }

  // Genera un mensaje JSON con un tipo y un payload (contenido)
  std::string make_message_json(const char* type, const std::string& payload){
    std::string j = "{\"type\":\"";
    j += type;
    j += "\",\"payload\":";
    j += payload; // payload ya es un objeto JSON valido
    j += "}";
    return j;
  }

} // namespace mp