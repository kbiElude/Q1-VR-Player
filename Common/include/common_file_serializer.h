/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(COMMON_FILE_SERIALIZER_H)
#define COMMON_FILE_SERIALIZER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

/* Forward decls */
class                                   FileSerializer;
typedef std::unique_ptr<FileSerializer> FileSerializerUniquePtr;

/* Helper struct defs */
#define MAX_VARIANT_SIZE (512)

typedef struct Variant
{
    enum class Type : uint8_t
    {
        FP32,
        I32,
        U8_TEXT_STRING,

        UNKNOWN
    } type;

    // Only used for u8_buffer values
    uint32_t size;

    union
    {
        float   fp32;
        int32_t i32;
        uint8_t u8_buffer[MAX_VARIANT_SIZE];
    } value;

    Variant()
        :size(0),
         type(Type::UNKNOWN)
    {
        /* Stub */
    }
} Variant;

#undef MAX_VARIANT_SIZE


/* Poor man's file-based data serializer. */
class FileSerializer
{
public:
    /* Public funcs */
    static FileSerializerUniquePtr create_for_reading(const char*                                 in_filename_ptr,
                                                      const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map);
    static FileSerializerUniquePtr create_for_writing(const char*                                 in_filename_ptr,
                                                      const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map);

    bool get_fp32          (const char*     in_key_ptr,
                            float*          out_value_ptr) const;
    bool get_i32           (const char*     in_key_ptr,
                            int*            out_value_ptr) const;
    bool get_u8_text_string(const char*     in_key_ptr,
                            const uint8_t** out_value_ptr_ptr,
                            uint32_t*       out_size_ptr) const;

    void set_fp32          (const char*     in_key_ptr,
                            const float&    in_value);
    void set_i32           (const char*     in_key_ptr,
                            const int&      in_value);
    void set_u8_text_string(const char*     in_key_ptr,
                            const uint8_t*  in_value_ptr,
                            const uint32_t& in_size);

    ~FileSerializer();

private:
    /* Private funcs */
    FileSerializer(const bool&                                 in_is_reader,
                   const char*                                 in_filename_ptr,
                   const std::map<std::string, Variant::Type>& in_setting_to_variant_type_map);

    bool init                    ();
    bool parse_serialized_data   (const std::vector<uint8_t>& in_data_u8_vec);
    void serialize_and_store_data();

    /* Private vars */
    const std::string m_filename;
    const bool        m_is_reader;

    std::map<std::string, Variant>             m_setting_to_variant_map;
    const std::map<std::string, Variant::Type> m_setting_to_variant_type_map;
};

#endif /* COMMON_FILE_SERIALIZER_H */