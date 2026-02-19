#ifndef numgeom_framework_vkutilities_h
#define numgeom_framework_vkutilities_h

#include "vulkan/vulkan.h"


//! Возвращает значение, не меньшее `s`, но кратное `byteAlign`.
//! Функцию используют для вычисления размера памяти с учетом выравнивания ее блоков.
inline VkDeviceSize Aligned(VkDeviceSize s, VkDeviceSize byteAlign)
{
    return (s + byteAlign - 1) & ~(byteAlign - 1);
}


const char* VkResultToString(VkResult);

const char* VkFormatToString(VkFormat);

#endif // !numgeom_framework_vkutilities_h
