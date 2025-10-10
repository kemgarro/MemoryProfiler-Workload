#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "BlockInfo.hpp"
namespace mp {

    // JSON plano: {"bytes_in_use":X,"peak":Y,"alloc_count":Z}
    std::string make_summary_json(std::size_t bytes_in_use,
                                  std::size_t peak,
                                  std::size_t alloc_count);

    // CSV plano (encabezado estable)
    // ptr,size,alloc_id,thread_id,t_ns,callsite
    std::string make_live_allocs_csv(const std::vector<BlockInfo>& blocks);

    // JSON: {"blocks":[{...}, ...]}
    std::string make_live_allocs_json(const std::vector<BlockInfo>& blocks);

    // Envoltura para GUI: {"type":"TYPE","payload":{...}}
    // payload_object_json DEBE ser un objeto JSON (sin comillas externas)
    std::string make_message_json(const char* type, const std::string& payload_object_json);

} // namespace mp