#ifndef numgeom_framework_vkutilities_h
#define numgeom_framework_vkutilities_h

#include "volk.h"

//! Возвращает значение, не меньшее `s`, но кратное `byteAlign`.
//! Функцию используют для вычисления размера памяти с учетом выравнивания ее
//! блоков.
inline VkDeviceSize Aligned(VkDeviceSize s, VkDeviceSize byteAlign) {
  return (s + byteAlign - 1) & ~(byteAlign - 1);
}

const char* VkResultToString(VkResult);

const char* VkFormatToString(VkFormat);

//! Проверяет, является ли значение `sample_count` допустимым флагом
//! VkSampleCountFlagBits (степень двойки от 1 до 64).
inline bool IsValidSampleCount(VkSampleCountFlagBits sample_count) {
  return sample_count == VK_SAMPLE_COUNT_1_BIT ||
         sample_count == VK_SAMPLE_COUNT_2_BIT ||
         sample_count == VK_SAMPLE_COUNT_4_BIT ||
         sample_count == VK_SAMPLE_COUNT_8_BIT ||
         sample_count == VK_SAMPLE_COUNT_16_BIT ||
         sample_count == VK_SAMPLE_COUNT_32_BIT ||
         sample_count == VK_SAMPLE_COUNT_64_BIT;
}

#endif  // !numgeom_framework_vkutilities_h
