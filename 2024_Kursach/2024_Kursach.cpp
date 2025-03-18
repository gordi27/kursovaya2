#include <windows.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

// операции над векторами
enum OperationType
{
    OP_MULTIPLY, // умножение на число
    OP_ADD, // сложение
    OP_SUBTRACT, // вычитание
    OP_SCALAR, // скалярное произведение
    OP_VECTOR_PRODUCT, // векторное произведение  
    OP_MIXED_PRODUCT // смешанное произведение
};

struct POINT2D
{
    int x1, y1; // начальная точка
    int x2, y2; // конечная точка
};

HINSTANCE hInst; // дескриптор текущего экземпляра приложения
const wchar_t ClassName[] = L"VectOpWindowClass"; // имя класса окна выбора
HWND hwndVectOP, hwndVectOp, hwndResult, hwndResultsText; // дескрипторы окон выбора и ввода/отображения векторов

OperationType selected_operation; // выбранная операция

// ---------------------------- Работа в 2D -------------------------------------------------

POINT2D vectors2D[3] =
{
    {150, 100, 50, 50},  // красный вектор 
    {50, 200, 200, 100},  // синий вектор
    {250, 300, 250, 100}   // зеленый вектор
};

POINT2D resultVectors2D[2][2]; // результирующие векторы для 2D
int result_scalar; // результат скалярного умножения

//----------------------------------------- Визуал -------------------------------------------

void drawVector(HDC hdc, double x1, double y1, double x2, double y2, COLORREF color, double scaleX, double scaleY, int originX, int originY) {
    HPEN hPen = CreatePen(PS_SOLID, 2, color);
    SelectObject(hdc, hPen);

    // Масштабируем координаты
    int scaledX1 = originX + static_cast<int>(x1 * scaleX);
    int scaledY1 = originY - static_cast<int>(y1 * scaleY);
    int scaledX2 = originX + static_cast<int>(x2 * scaleX);
    int scaledY2 = originY - static_cast<int>(y2 * scaleY);

    MoveToEx(hdc, scaledX1, scaledY1, NULL); // начальная точка вектора
    LineTo(hdc, scaledX2, scaledY2); // конечная точка вектора
    DeleteObject(hPen);
}


void drawCoordinateAxes(HDC hdc, RECT rect, double& scaleX, double& scaleY, int& originX, int& originY) {
    // Рисуем оси координат
    HPEN hPenAxis = CreatePen(PS_SOLID, 2, RGB(0, 0, 0)); // черный цвет для осей
    SelectObject(hdc, hPenAxis);

    // Размеры клиентской области
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Отступы от границ окна
    int marginLeft = 10;
    int marginBottom = 20;

    // Координаты для осей с учетом отступов
    originX = marginLeft;
    originY = height - marginBottom;

    // Область допустимых значений
    double minX = 0.0;
    double maxX = 650.0; // Примерное максимальное значение по X
    double minY = 0.0;
    double maxY = 650.0; // Примерное максимальное значение по Y

    // Масштабные коэффициенты
    scaleX = (double)(width - marginLeft) / (maxX - minX);
    scaleY = (double)(height - marginBottom) / (maxY - minY);

    // Ось X (вдоль нижнего края окна)
    MoveToEx(hdc, originX, originY, NULL); // начальная точка
    LineTo(hdc, width, originY); // конечная точка

    // Ось Y (вдоль левого края окна)
    MoveToEx(hdc, originX, originY, NULL); // начальная точка
    LineTo(hdc, originX, 0); // конечная точка

    // рисуем отметки и подписи на оси X
    for (int i = 0; i <= maxX; i += 50) {
        int x = originX + (int)(i * scaleX);
        MoveToEx(hdc, x, originY - 5, NULL); // начальная точка отметки на оси X
        LineTo(hdc, x, originY + 5); // конечная точка отметки на оси X

        wstring label = to_wstring(i); // создаем строку с подписью
        TextOut(hdc, x - 10, originY + 5, label.c_str(), label.length()); // выводим подпись
    }

    // рисуем отметки и подписи на оси Y
    for (int i = 0; i <= maxY; i += 50) {
        int y = originY - (int)(i * scaleY);
        MoveToEx(hdc, originX - 5, y, NULL); // начальная точка отметки на оси Y
        LineTo(hdc, originX + 5, y); // конечная точка отметки на оси Y

        wstring label = to_wstring(i); // создаем строку с подписью
        TextOut(hdc, originX + 5, y - 10, label.c_str(), label.length()); // выводим подпись
    }

    DeleteObject(hPenAxis); // удаляем объект пера для осей
}



//------------------------------------ Запись результатов --------------------------------

// преобразование wstring в string
string wstring_to_string(const wstring& wstr)
{
    return string(wstr.begin(), wstr.end());
}

void AddTextToResultsWindow(const wchar_t* text)
{
    // находим дескриптор текстового поля в окне ResultsText
    HWND hwndResultsEdit = FindWindowEx(hwndResultsText, NULL, L"EDIT", NULL);

    // добавляем текст в текстовое поле
    SendMessage(hwndResultsEdit, EM_SETSEL, -1, -1); // Перемещаем курсор в конец текста
    SendMessage(hwndResultsEdit, EM_REPLACESEL, TRUE, (LPARAM)text); // Добавляем текст
    SendMessage(hwndResultsEdit, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n"); // Переходим на новую строку
}


//------------------------------------ Операции -----------------------------------------

// сложение
POINT2D addVectors(const POINT2D& v1, const POINT2D& v2, const POINT2D& v3) {
    POINT2D result;
    result.x1 = v1.x1 + v2.x1 + v3.x1; // cложение координат начальной точки векторов
    result.y1 = v1.y1 + v2.y1 + v3.y1;
    result.x2 = v1.x2 + v2.x2 + v3.x2; // cложение координат конечной точки векторов
    result.y2 = v1.y2 + v2.y2 + v3.y2;
    return result;
}

// вычитание
POINT2D subtractVectors(const POINT2D& v1, const POINT2D& v2) {
    POINT2D result;
    result.x1 = v1.x1 - v2.x1; // вычитание координат начальной точки векторов
    result.y1 = v1.y1 - v2.y1;
    result.x2 = v1.x2 - v2.x2; // вычитание координат конечной точки векторов
    result.y2 = v1.y2 - v2.y2;
    return result;
}

// нахождение вектора с макс. координатами
POINT2D findVectorWithMaxCoordinates(const POINT2D vectors[], int size) {
    POINT2D maxVector = vectors[0];
    for (int i = 1; i < size; ++i) {
        if ((vectors[i].x1 > maxVector.x1) && (vectors[i].y1 > maxVector.y1) && (vectors[i].x2 > maxVector.x2) && (vectors[i].y2 > maxVector.y2)) {
            maxVector = vectors[i];
        }
    }
    return maxVector;
}

// нахождение вектора с мин. координатами
POINT2D findVectorWithMinCoordinates(const POINT2D vectors[], int size) {
    POINT2D minVector = vectors[0];
    for (int i = 1; i < size; ++i) {
        if ((vectors[i].x1 < minVector.x1) && (vectors[i].y1 < minVector.y1) && (vectors[i].x2 < minVector.x2) && (vectors[i].y2 < minVector.y2)) {
            minVector = vectors[i];
        }
    }
    return minVector;
}

// умножение на число
POINT2D multiplyVectorByScalar(const POINT2D& v, double scalar) {
    POINT2D result;
    // умножаем каждую координату на скаляр и приводим к int
    result.x1 = static_cast<int>(v.x1 * scalar);
    result.y1 = static_cast<int>(v.y1 * scalar);
    result.x2 = static_cast<int>(v.x2 * scalar);
    result.y2 = static_cast<int>(v.y2 * scalar);
    return result;
}

// скалярное произведение
int scalarProduct(const POINT2D& v1, const POINT2D& v2) {
    return (v1.x2 - v1.x1) * (v2.x2 - v2.x1) + (v1.y2 - v1.y1) * (v2.y2 - v2.y1);
}

// ---------------------------- Работа в 3D -------------------------------------------------

struct POINT3D
{
    int x1, y1, z1;
    int x2, y2, z2;
};

POINT3D vectors3D[3] =
{
    {150, 100, 10, 50, 50, 20},  // красный вектор 
    {50, 200, 20, 200, 100, 30},   // синий вектор
    {250, 300, 250, 100, 60, 40}   // зеленый вектор
};

// векторное произведение
POINT3D vectorProduct(const POINT3D vectors[], int size) {
    POINT3D resultVector;
    // вычисляем компоненты x, y, z результирующего вектора
    resultVector.x1 = vectors[0].y1 * vectors[1].z1 - vectors[0].z1 * vectors[1].y1;
    resultVector.y1 = vectors[0].z1 * vectors[1].x1 - vectors[0].x1 * vectors[1].z1;
    resultVector.z1 = vectors[0].x1 * vectors[1].y1 - vectors[0].y1 * vectors[1].x1;

    resultVector.x2 = vectors[0].y2 * vectors[1].z2 - vectors[0].z2 * vectors[1].y2;
    resultVector.y2 = vectors[0].z2 * vectors[1].x2 - vectors[0].x2 * vectors[1].z2;
    resultVector.z2 = vectors[0].x2 * vectors[1].y2 - vectors[0].y2 * vectors[1].x2;

    return resultVector;
}

// смешанное произведение
int mixedProduct(const POINT3D& v1, const POINT3D& v2, const POINT3D& v3) {
    // вычисляем векторное произведение v2 и v3
    int v2_cross_v3_x = v2.y1 * v3.z1 - v3.y1 * v2.z1;
    int v2_cross_v3_y = v2.z1 * v3.x1 - v3.z1 * v2.x1;
    int v2_cross_v3_z = v2.x1 * v3.y1 - v3.x1 * v2.y1;

    // вычисляем скалярное произведение v1 и вектора, полученного в результате векторного произведения v2 и v3
    int result = v1.x1 * v2_cross_v3_x + v1.y1 * v2_cross_v3_y + v1.z1 * v2_cross_v3_z;

    return result;
}

//---------------------------------------- Окна ---------------------------------------------

// обработка сообщений для окна с векторами и операциями
LRESULT CALLBACK VectOpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndMultiplyButton, hwndAddButton, hwndSubtractButton,
        hwndScalarButton, hwndVectorProductButton, hwndMixedProductButton;
    static HWND hwndInputField, hwndInputButton;

    switch (msg)
    {
    case WM_CREATE: { // обработка создания окна
        hwndMultiplyButton = CreateWindow(L"BUTTON", L"Умножение на число", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 10, 190, 30, hwnd, (HMENU)OP_MULTIPLY, hInst, NULL);
        hwndAddButton = CreateWindow(L"BUTTON", L"Сложение", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 50, 190, 30, hwnd, (HMENU)OP_ADD, hInst, NULL);
        hwndSubtractButton = CreateWindow(L"BUTTON", L"Вычитание", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 90, 190, 30, hwnd, (HMENU)OP_SUBTRACT, hInst, NULL);
        hwndScalarButton = CreateWindow(L"BUTTON", L"Скалярное произведение", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 130, 190, 30, hwnd, (HMENU)OP_SCALAR, hInst, NULL);
        hwndVectorProductButton = CreateWindow(L"BUTTON", L"Векторное произведение", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 170, 190, 30, hwnd, (HMENU)OP_VECTOR_PRODUCT, hInst, NULL);
        hwndMixedProductButton = CreateWindow(L"BUTTON", L"Смешанное произведение", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            690, 210, 190, 30, hwnd, (HMENU)OP_MIXED_PRODUCT, hInst, NULL);
        break;
    }

    case WM_SIZE: { // обработка изменения размера окна
        // получаем новые размеры клиентской области окна
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right;

        // расстояние от правой границы окна до правой границы кнопок
        const int rightMargin = 10;

        // устанавливаем новые позиции кнопок
        SetWindowPos(hwndMultiplyButton, NULL, clientWidth - 190 - rightMargin, 10, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hwndAddButton, NULL, clientWidth - 190 - rightMargin, 50, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hwndSubtractButton, NULL, clientWidth - 190 - rightMargin, 90, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hwndScalarButton, NULL, clientWidth - 190 - rightMargin, 130, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hwndVectorProductButton, NULL, clientWidth - 190 - rightMargin, 170, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hwndMixedProductButton, NULL, clientWidth - 190 - rightMargin, 210, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        InvalidateRect(hwnd, NULL, TRUE); // перерисовываем окно
        break;
    }


    case WM_COMMAND: { // обработка команды
        selected_operation = (OperationType)LOWORD(wParam);

        // выбранная операция
        switch (selected_operation)
        {
        case OP_MULTIPLY: // умножение на число
        case OP_ADD: // сложение
        case OP_SUBTRACT: // вычитание
            hwndResult = CreateWindow(L"ResultWindowClass", L"Результат операции",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                900, 710, NULL, NULL, hInst, NULL);
            ShowWindow(hwndResult, SW_SHOW);
            UpdateWindow(hwndResult);
            break;

        case OP_SCALAR: { // скалярное произведение
            int result_scalar = scalarProduct(vectors2D[0], vectors2D[1]);
            // добавляем результат в окно ResultsText
            wchar_t buffer[50];
            swprintf_s(buffer, L"Скалярное произведение: %d", result_scalar);
            AddTextToResultsWindow(buffer);

            // сохраняем результаты в файл
            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                outFile << "Скалярное произведение:" << endl; // запись на русском
                outFile << result_scalar << endl; // запись результата
                outFile.close();
            }
            break;
        }


        case OP_VECTOR_PRODUCT: { // векторное произведение
            CreateWindow(L"MessageWindowClass", L"Сообщение", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 250, 100, NULL, NULL, hInst, NULL);

            POINT3D resultVector = vectorProduct(vectors3D, 3);

            // добавляем результат в окно ResultsText
            wchar_t buffer[200];  // увеличиваем размер буфера для хранения строки
            swprintf_s(buffer, 200, L"Векторное произведение: (%d, %d, %d) ; (%d, %d, %d)",
                resultVector.x1, resultVector.y1, resultVector.z1,
                resultVector.x2, resultVector.y2, resultVector.z2);
            AddTextToResultsWindow(buffer);

            // сохранение результата в файл
            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                outFile << "Векторное произведение:" << endl;
                outFile << "(" << resultVector.x1 << ", " << resultVector.y1 << ", " << resultVector.z1 << ") ; ("
                    << resultVector.x2 << ", " << resultVector.y2 << ", " << resultVector.z2 << ")" << endl;
                outFile.close();
            }
            break;
        }

        case OP_MIXED_PRODUCT: { // смешанное произведение
            CreateWindow(L"MessageWindowClass", L"Сообщение", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 250, 100, NULL, NULL, hInst, NULL);

            // вычисляем смешанное произведение трех векторов
            int resultMixedProduct = mixedProduct(vectors3D[0], vectors3D[1], vectors3D[2]);

            // добавляем результат в окно ResultsText
            wchar_t buffer[50];
            swprintf_s(buffer, L"Смешанное произведение: %d", resultMixedProduct);
            AddTextToResultsWindow(buffer);

            // сохранение результата в файл
            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                outFile << "Смешанное произведение:" << endl;
                outFile << resultMixedProduct << endl;
                outFile.close();
            }
            break;
        }
        }

        break;
    }

    case WM_PAINT: { // прорисовка
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        double scaleX, scaleY;
        int originX, originY;
        drawCoordinateAxes(hdc, rect, scaleX, scaleY, originX, originY);

        // рисуем исходные векторы
        drawVector(hdc, vectors2D[0].x1, vectors2D[0].y1, vectors2D[0].x2, vectors2D[0].y2, RGB(255, 0, 0), scaleX, scaleY, originX, originY); // красный вектор
        drawVector(hdc, vectors2D[1].x1, vectors2D[1].y1, vectors2D[1].x2, vectors2D[1].y2, RGB(0, 0, 255), scaleX, scaleY, originX, originY); // синий вектор
        drawVector(hdc, vectors2D[2].x1, vectors2D[2].y1, vectors2D[2].x2, vectors2D[2].y2, RGB(0, 255, 0), scaleX, scaleY, originX, originY); // зеленый вектор

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY: { // обработка уничтожения окна
        PostQuitMessage(0);
        break;
    }

    default: {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    }
    return 0;
}

// обработка сообщений для окна с результирующим вектором
LRESULT CALLBACK ResultWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static vector<string> results;
    static HWND hwndInputField, hwndInputButton;
    static double scalarInput;

    switch (msg)
    {
    case WM_CREATE: {
        if (selected_operation == OP_MULTIPLY) { // умножение на число
            hwndInputField = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                750, 30, 50, 20, hwnd, NULL, hInst, NULL);
            hwndInputButton = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                750, 60, 50, 30, hwnd, (HMENU)1, hInst, NULL);
        }
        break;
    }

    case WM_SIZE: { // обработка изменения размера окна
        InvalidateRect(hwnd, NULL, TRUE); // Перерисовываем окно
        break;
    }

    case WM_COMMAND: { // обработка команды
        if (LOWORD(wParam) == 1) {
            wchar_t scalarText[10];
            GetWindowText(hwndInputField, scalarText, 10);
            scalarInput = _wtof(scalarText); // конвертируем текст введенный пользователем в число
            InvalidateRect(hwnd, NULL, TRUE); // перерисовываем окно для отображения результата

            // результаты умножения каждого вектора на скаляр
            POINT2D resultRed = multiplyVectorByScalar(vectors2D[0], scalarInput);
            POINT2D resultBlue = multiplyVectorByScalar(vectors2D[1], scalarInput);
            POINT2D resultGreen = multiplyVectorByScalar(vectors2D[2], scalarInput);

            // добавляем результаты умножения в окно ResultsText
            wchar_t buffer[100];
            swprintf_s(buffer, L"Красный вектор после умножения: (%d, %d) ; (%d, %d)", resultRed.x1, resultRed.y1, resultRed.x2, resultRed.y2);
            AddTextToResultsWindow(buffer);
            swprintf_s(buffer, L"Синий вектор после умножения: (%d, %d) ; (%d, %d)", resultBlue.x1, resultBlue.y1, resultBlue.x2, resultBlue.y2);
            AddTextToResultsWindow(buffer);
            swprintf_s(buffer, L"Зеленый вектор после умножения: (%d, %d) ; (%d, %d)", resultGreen.x1, resultGreen.y1, resultGreen.x2, resultGreen.y2);
            AddTextToResultsWindow(buffer);

            // сохраняем результаты в файл
            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                outFile << "Красный вектор после умножения на число:" << endl;
                outFile << "(" << resultRed.x1 << ", " << resultRed.y1 << ") ; (" << resultRed.x2 << ", " << resultRed.y2 << ")" << endl;
                outFile << "Синий вектор после умножения на число:" << endl;
                outFile << "(" << resultBlue.x1 << ", " << resultBlue.y1 << ") ; (" << resultBlue.x2 << ", " << resultBlue.y2 << ")" << endl;
                outFile << "Зеленый вектор после умножения на число:" << endl;
                outFile << "(" << resultGreen.x1 << ", " << resultGreen.y1 << ") ; (" << resultGreen.x2 << ", " << resultGreen.y2 << ")" << endl;
                outFile.close();
            }
            break;
        }
        break;
    }

    case WM_PAINT: { // обработка рисования окна
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        double scaleX, scaleY;
        int originX, originY;
        drawCoordinateAxes(hdc, rect, scaleX, scaleY, originX, originY);

        switch (selected_operation)
        {
        case OP_MULTIPLY: { // умножение на число
            // результаты умножения каждого вектора на скаляр
            POINT2D resultRed = multiplyVectorByScalar(vectors2D[0], scalarInput);
            POINT2D resultBlue = multiplyVectorByScalar(vectors2D[1], scalarInput);
            POINT2D resultGreen = multiplyVectorByScalar(vectors2D[2], scalarInput);

            // рисуем вектора после умножения
            drawVector(hdc, resultRed.x1, resultRed.y1, resultRed.x2, resultRed.y2, RGB(255, 0, 0), scaleX, scaleY, originX, originY); // красный
            drawVector(hdc, resultBlue.x1, resultBlue.y1, resultBlue.x2, resultBlue.y2, RGB(0, 0, 255), scaleX, scaleY, originX, originY); // синий
            drawVector(hdc, resultGreen.x1, resultGreen.y1, resultGreen.x2, resultGreen.y2, RGB(0, 255, 0), scaleX, scaleY, originX, originY); // зеленый

            break;
        }

        case OP_SUBTRACT: { // вычитание
            POINT2D maxVector = findVectorWithMaxCoordinates(vectors2D, 3);
            POINT2D minVector = findVectorWithMinCoordinates(vectors2D, 3);
            POINT2D result = subtractVectors(maxVector, minVector);

            drawVector(hdc, result.x1, result.y1, result.x2, result.y2, RGB(128, 0, 128), scaleX, scaleY, originX, originY); // фиолетовый цвет

            wchar_t buffer[100];
            swprintf_s(buffer, L"Результат вычитания: (%d, %d) ; (%d, %d)", result.x1, result.y1, result.x2, result.y2);
            AddTextToResultsWindow(buffer);

            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                // записываем результат операции в файл
                outFile << "Вычитание:" << endl;
                outFile << "(" << result.x1 << ", " << result.y1 << ") ; (" << result.x2 << ", " << result.y2 << ")" << endl;
                outFile.close();
            }
            break;
        }

        case OP_ADD: { // сложение
            POINT2D result = addVectors(vectors2D[0], vectors2D[1], vectors2D[2]);
            drawVector(hdc, result.x1, result.y1, result.x2, result.y2, RGB(128, 0, 128), scaleX, scaleY, originX, originY);

            wchar_t buffer[100];
            swprintf_s(buffer, L"Результат сложения: (%d, %d) ; (%d, %d)", result.x1, result.y1, result.x2, result.y2);
            AddTextToResultsWindow(buffer);

            ofstream outFile("results.txt", ios::app);
            if (outFile.is_open())
            {
                // записываем результат операции в файл
                outFile << "Сложение:" << endl;
                outFile << "(" << result.x1 << ", " << result.y1 << ") ; (" << result.x2 << ", " << result.y2 << ")" << endl;
                outFile.close();
            }
            break;
        }

        }

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY: { // обработка уничтожения окна
        PostQuitMessage(0);
        break;
    }

    default: {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    }
    return 0;
}

LRESULT CALLBACK ResultsTextWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        // создаем текстовое поле, где будут отображаться результаты
        CreateWindow(L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 360, 240, hwnd, NULL, NULL, NULL);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}


ATOM MyRegisterClass(HINSTANCE hInstance, LPCWSTR className, WNDPROC wndProc)
{
    WNDCLASS wc = {}; // экземпляр структуры WNDCLASS
    wc.lpfnWndProc = wndProc; // указатель на оконную процедуру
    wc.hInstance = hInstance; // дескриптор текущего экземпляра приложения
    wc.lpszClassName = className; // имя класса окна
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // курсор для окна
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // фон окна

    return RegisterClass(&wc); // регистрация класса окна
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance; // дескриптор текущего экземпляра приложения

    // регистрация классов окон
    MyRegisterClass(hInstance, ClassName, VectOpWndProc);
    MyRegisterClass(hInstance, L"ResultWindowClass", ResultWndProc);
    MyRegisterClass(hInstance, L"ResultsTextWindowClass", ResultsTextWndProc);

    // основное окно приложения
    hwndVectOP = CreateWindow(ClassName, L"Начало",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        900, 750, NULL, NULL, hInstance, NULL);

    if (!hwndVectOP) // успешность создания окна
    {
        return FALSE;
    }

    // отображаем и обновляем основное окно
    ShowWindow(hwndVectOP, nCmdShow);
    UpdateWindow(hwndVectOP);

    hwndResultsText = CreateWindow(L"ResultsTextWindowClass", L"Results", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, hInstance, NULL);

    MSG msg;
    // цикл обработки сообщений
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg); // переводим неклавиатурные сообщения в символы
        DispatchMessage(&msg); // отправляем сообщение оконной процедуре
    }

    return (int)msg.wParam;
}