#include <windows.h>
#include <iostream>
#include <queue>

std::queue<int> Buffer;
HANDLE semaphoreForConsumer;
HANDLE semaphoreForProducer;
HANDLE mutexForBuffer;
int B_SIZE, P_SLEEP, C_SLEEP, I_COUNT;
int remainingItems;

DWORD WINAPI ToProduce(LPVOID);
DWORD WINAPI ToConsume(LPVOID);

int main() {
    setlocale(LC_ALL, "Russian");
    DWORD ProducerThreadId, ConsumerThreadId;

    std::cout << " ¬ведите количество товаров, которое нужно произвести : ";
    std::cin >> I_COUNT;
    std::cout << " ¬ведите размер буфера (не больше, чем " << I_COUNT << ") : ";
    std::cin >> B_SIZE;
    std::cout << " ¬ведите врем€ между выпусками единицы товара (мс) : ";
    std::cin >> P_SLEEP;
    std::cout << " ¬ведите врем€ между потреблени€ми единицы товара (ms) : ";
    std::cin >> C_SLEEP;

    if (B_SIZE > I_COUNT) {
        std::cout << " –азмер буфера не может быть больше количества вещей, которое нужно произвести.";
        return 1;
    }

    semaphoreForConsumer = CreateSemaphore(NULL, 0, B_SIZE, NULL);
    semaphoreForProducer = CreateSemaphore(NULL, B_SIZE, B_SIZE, NULL);
    mutexForBuffer = CreateMutex(NULL, FALSE, NULL);

    remainingItems = I_COUNT;

    HANDLE hProducer = CreateThread(NULL, 0, ToProduce, NULL, 0, &ProducerThreadId);
    HANDLE hConsumer = CreateThread(NULL, 0, ToConsume, NULL, 0, &ConsumerThreadId);

    WaitForSingleObject(hProducer, INFINITE);
    WaitForSingleObject(hConsumer, INFINITE);
    CloseHandle(semaphoreForConsumer);
    CloseHandle(semaphoreForProducer);
    CloseHandle(mutexForBuffer);
    CloseHandle(hProducer);
    CloseHandle(hConsumer);

    return 0;
}

DWORD WINAPI ToProduce(LPVOID lpParam) {
    for (int i = 0; i < I_COUNT; ++i) {
        WaitForSingleObject(semaphoreForProducer, INFINITE);

        WaitForSingleObject(mutexForBuffer, INFINITE);
        Buffer.push(i);
        std::cout << " ѕроизведено товаров на текущий момент : " << i + 1 << std::endl;
        ReleaseMutex(mutexForBuffer);

        ReleaseSemaphore(semaphoreForConsumer, 1, NULL);
        Sleep(P_SLEEP);
    }

    return 0;
}

DWORD WINAPI ToConsume(LPVOID lpParam) {
    while (true) {
        WaitForSingleObject(semaphoreForConsumer, INFINITE);

        WaitForSingleObject(mutexForBuffer, INFINITE);
        if (!Buffer.empty()) {
            int item = Buffer.front();
            Buffer.pop();
            std::cout << " ѕотреблено товаров на текущий момент : " << item + 1 << std::endl;

            remainingItems--;

            ReleaseMutex(mutexForBuffer);

            ReleaseSemaphore(semaphoreForProducer, 1, NULL);
        }
        else {
            ReleaseMutex(mutexForBuffer);
        }

        Sleep(C_SLEEP);

        if (remainingItems == 0) {
            break;
        }
    }

    return 0;
}
