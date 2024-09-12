/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

// Yes, thank you.
#define _CRT_SECURE_NO_WARNINGS

#include "common_file_serializer.h"
#include <sys/stat.h>
#include "VRPlayer_types.h"



FileSerializer::FileSerializer(const bool&                                 in_is_reader,
                               const char*                                 in_filename_ptr,
                               const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map)
    :m_filename                   (in_filename_ptr),
     m_is_reader                  (in_is_reader),
     m_setting_to_variant_type_map(in_setting_to_variant_type_map)
{
    /* Stub */
}

FileSerializer::~FileSerializer()
{
    if (m_is_reader == false)
    {
        serialize_and_store_data();
    }
}

FileSerializerUniquePtr FileSerializer::create_for_reading(const char*                                 in_filename_ptr,
                                                           const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map)
{
    FileSerializerUniquePtr result_ptr(
        new FileSerializer(true, /* in_is_reader */
                           in_filename_ptr,
                           in_setting_to_variant_type_map)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

FileSerializerUniquePtr FileSerializer::create_for_writing(const char*                                 in_filename_ptr,
                                                           const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map)
{
    FileSerializerUniquePtr result_ptr(
        new FileSerializer(false, /* in_is_reader */
                           in_filename_ptr,
                           in_setting_to_variant_type_map)
    );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool FileSerializer::get_fp32(const char* in_key_ptr,
                              float*      out_value_ptr) const
{
    auto map_iterator = m_setting_to_variant_map.find(in_key_ptr);
    bool result       = false;

    if (map_iterator != m_setting_to_variant_map.end() )
    {
        AI_ASSERT(map_iterator->second.type == Variant::Type::FP32);

        *out_value_ptr = map_iterator->second.value.fp32;
         result        = true;
    }

    return result;
}

bool FileSerializer::get_i32(const char* in_key_ptr,
                             int*        out_value_ptr) const
{
    auto map_iterator = m_setting_to_variant_map.find(in_key_ptr);
    bool result       = false;

    if (map_iterator != m_setting_to_variant_map.end() )
    {
        AI_ASSERT(map_iterator->second.type == Variant::Type::I32);

        *out_value_ptr = map_iterator->second.value.i32;
         result        = true;
    }

    return result;
}

bool FileSerializer::init()
{
    FILE* file_ptr = nullptr;
    bool  result   = false;

    {
        const auto mode_ptr = (m_is_reader) ? "r"
                                            : "w";

        file_ptr = ::fopen(m_filename.c_str(),
                           mode_ptr);
    }

    if (file_ptr == nullptr)
    {
        goto end;
    }

    if (m_is_reader)
    {
        std::vector<uint8_t> data_u8_vec;
        size_t               file_size  = 0;
        struct stat          file_stats = {};

        if (::fstat(::_fileno(file_ptr),
                    &file_stats) == -1)
        {
            goto end;
        }

        file_size = file_stats.st_size;

        data_u8_vec.resize(file_size);

        if (::fread(data_u8_vec.data(),
                    file_size,
                    1,
                    file_ptr) != 1)
        {
            goto end;
        }

        if (!parse_serialized_data(data_u8_vec) )
        {
            goto end;
        }
    }

    result = true;
end:
    if (file_ptr != nullptr)
    {
        ::fclose(file_ptr);

        file_ptr = nullptr;
    }

    return result;
}

bool FileSerializer::parse_serialized_data(const std::vector<uint8_t>& in_u8_vec)
{
    auto input_iterator = in_u8_vec.begin();
    bool result         = false;

    /* 1. Convert textual representation to an array of key-value mappings. */
    typedef struct
    {
        std::string key;
        std::string value;
    } LineData;

    std::vector<LineData> line_data_vec;

    {
        std::string current_key;
        std::string current_value_string;

        do
        {
            auto setter_char_iterator = std::find(input_iterator,
                                                  in_u8_vec.end(),
                                                  '=');
            auto eol_iterator         = std::find(input_iterator,
                                                  in_u8_vec.end(),
                                                  '\n');

            if (setter_char_iterator == in_u8_vec.end() )
            {
                break;
            }

            current_key          = std::string(input_iterator,       setter_char_iterator - 1);
            current_value_string = std::string(setter_char_iterator, eol_iterator);

            line_data_vec.emplace_back(
                LineData{current_key,
                         current_value_string}
            );
        }
        while (true);
    }

    /* 2. Parse values for recognized keys. */

    for (const auto& current_line_data : line_data_vec)
    {
        auto map_iterator = m_setting_to_variant_type_map.find(current_line_data.key);

        if (map_iterator == m_setting_to_variant_type_map.end() )
        {
            continue;
        }

        m_setting_to_variant_map[map_iterator->first].type = map_iterator->second;

        switch (map_iterator->second)
        {
            case Variant::Type::FP32:
            {
                sscanf(map_iterator->first.c_str(),
                       "%f",
                       &m_setting_to_variant_map[map_iterator->first].value.fp32);

                break;
            }

            case Variant::Type::I32:
            {
                sscanf(map_iterator->first.c_str(),
                       "%d",
                       &m_setting_to_variant_map[map_iterator->first].value.i32);

                break;
            }

            default:
            {
                AI_ASSERT_FAIL();
            }
        }
    }

    result = true;
    return result;
}

void FileSerializer::serialize_and_store_data()
{
    FILE* file_ptr = ::fopen(m_filename.c_str(),
                             "w");

    if (file_ptr != nullptr)
    {
        for (const auto& current_setting : m_setting_to_variant_map)
        {
            switch (current_setting.second.type)
            {
                case Variant::Type::FP32:
                {
                    fprintf(file_ptr,
                            "%s=%f\n",
                            current_setting.first.c_str(),
                            current_setting.second.value.fp32);

                    break;
                }

                case Variant::Type::I32:
                {
                    fprintf(file_ptr,
                            "%s=%d\n",
                            current_setting.first.c_str(),
                            current_setting.second.value.i32);

                    break;
                }

                default:
                {
                    AI_ASSERT_FAIL();
                }
            }
        }

        ::fclose(file_ptr);
    }
}

void FileSerializer::set_fp32(const char*  in_key_ptr,
                              const float& in_value)
{
    AI_ASSERT(m_setting_to_variant_type_map.find(in_key_ptr) != m_setting_to_variant_type_map.end() &&
              m_setting_to_variant_type_map.at  (in_key_ptr) == Variant::Type::FP32);

    m_setting_to_variant_map[in_key_ptr].type       = Variant::Type::FP32;
    m_setting_to_variant_map[in_key_ptr].value.fp32 = in_value;
}

void FileSerializer::set_i32(const char* in_key_ptr,
                             const int&  in_value)
{
    AI_ASSERT(m_setting_to_variant_type_map.find(in_key_ptr) != m_setting_to_variant_type_map.end() &&
              m_setting_to_variant_type_map.at  (in_key_ptr) == Variant::Type::I32);

    m_setting_to_variant_map[in_key_ptr].type      = Variant::Type::I32;
    m_setting_to_variant_map[in_key_ptr].value.i32 = in_value;
}