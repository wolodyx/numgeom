#!/usr/bin/python3

import sys

def binary_to_hex_array(filename, chunk_size=4):
    """
    Читает двоичный файл и преобразует в массив шестнадцатеричных чисел

    Args:
        filename (str): путь к двоичному файлу
        chunk_size (int): размер блока в байтах (по умолчанию 4)

    Returns:
        str: строка с шестнадцатеричными числами, разделенными запятыми
    """
    try:
        with open(filename, 'rb') as file:
            data = file.read()
            hex_array = []
            for i in range(0, len(data), chunk_size):
                chunk = data[i:i+chunk_size][::-1]
                hex_value = chunk.hex()
                hex_array.append(f"0x{hex_value}")

            return ', '.join(hex_array)

    except FileNotFoundError:
        return f"Ошибка: Файл '{filename}' не найден"
    except Exception as e:
        return f"Ошибка при обработке файла: {str(e)}"

def main():
    if len(sys.argv) != 3:
        print("usage: python binary_to_hex.py <binary_file_name> <output_file_name>")
        sys.exit(1)

    binary_filename = sys.argv[1]
    result = binary_to_hex_array(binary_filename, 4)

    out_filename = sys.argv[2]
    with open(out_filename, "w") as file:
        file.write(result)

if __name__ == "__main__":
    main()

