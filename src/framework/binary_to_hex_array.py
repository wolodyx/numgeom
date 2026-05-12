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
    import argparse

    parser = argparse.ArgumentParser(
        description='Reads a binary file and converts it into an array of hexadecimal numbers'
    )
    parser.add_argument(
        'binary_filename',
        help='the path to the binary file'
    )
    parser.add_argument(
        '--output',
        dest='out_filename',
        required=True,
        help='the path to the output file'
    )
    parser.add_argument(
        '--chunk-size',
        dest='chunk_size',
        type=int,
        default=4,
        help='block size in bytes (default is 4)'
    )

    args = parser.parse_args()

    result = binary_to_hex_array(args.binary_filename, args.chunk_size)

    with open(args.out_filename, "w") as file:
        file.write(result)

if __name__ == "__main__":
    main()
