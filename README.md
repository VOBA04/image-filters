# SatImageFilter

Проект по реализации операторов Собеля и Превитта для обработки TIFF-файлов.

## Описание

Этот проект предоставляет инструменты для обработки изображений в формате TIFF с использованием операторов Собеля и Превитта. Эти операторы используются для выделения границ в изображениях.

## Использованные технологии

- **C++17**: Современный стандарт языка программирования C++.
- **CMake 3.30.5**: Система автоматизации сборки.
- **libtiff 4.7.0**: Библиотека для работы с изображениями в формате TIFF.
- **Doxygen**: Инструмент для генерации документации из исходного кода.

## Генерация документации

Для генерации документации с использованием Doxygen выполните следующие шаги:

1. Установите Doxygen, если он еще не установлен. Инструкции по установке можно найти на [официальном сайте Doxygen](http://www.doxygen.nl/download.html).
2. Убедитесь, что файл конфигурации `Doxyfile` находится в корневом каталоге проекта и настроен.
3. Запустите Doxygen для генерации документации:

   ```sh
   doxygen Doxyfile
   ```

4. Сгенерированная документация будет находиться в каталоге `html` (или другом, указанном в конфигурации).

Теперь вы можете открыть файл `index.html` в браузере для просмотра документации.

## Руководство по структуре папок

При первом запуске программы создаются следующие папки:

- `images/`: Корневая папка для изображений.
  - `images/original/`: Папка для исходных изображений.
  - `images/prewitt/`: Папка для изображений, обработанных оператором Превитта.
  - `images/sobel/`: Папка для изображений, обработанных оператором Собеля.
  - `images/gaussian/`: Папка для изображений, обработанных фильтром Гаусса.

### Размещение изображений

1. Поместите исходные изображения в папку `images/original/`.
2. Запустите программу для обработки изображений.
3. Обработанные изображения будут сохранены в папки `images/prewitt/`, `images/sobel/` и `images/gaussian/` соответственно.

### Файл kernel.txt

Файл `kernel.txt` используется для задания произвольного ядра свертки. Он должен находиться в корневом каталоге проекта и содержать следующие данные:

1. Размер ядра (нечетное число).
2. Флаг, указывающий, можно ли вращать ядро (0 или 1).
3. Значения ядра, записанные построчно.

Пример содержимого файла `kernel.txt`:

```plaintext
3
1
-1 0 1
-2 0 2
-1 0 1
```

Этот пример задает ядро размером 3x3, которое можно вращать, с указанными значениями.

## TODO

- ~~Добавить размытие перед обработкой и поддержку произвольной матрицы.~~
- Сделать покрытие тестами.
- Реализовать свертку с использованием CUDA.
- Провести оптимизацию.
- Реализовать простой графический интерфейс (для ЖЦРПО).
