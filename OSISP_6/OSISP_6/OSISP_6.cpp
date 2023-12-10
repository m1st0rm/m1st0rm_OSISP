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

    std::cout << " ������� ���������� �������, ������� ����� ���������� : ";
    std::cin >> I_COUNT;
    std::cout << " ������� ������ ������ (�� ������, ��� " << I_COUNT << ") : ";
    std::cin >> B_SIZE;
    std::cout << " ������� ����� ����� ��������� ������� ������ (��) : ";
    std::cin >> P_SLEEP;
    std::cout << " ������� ����� ����� ������������� ������� ������ (ms) : ";
    std::cin >> C_SLEEP;

    if (B_SIZE > I_COUNT) {
        std::cout << " ������ ������ �� ����� ���� ������ ���������� �����, ������� ����� ����������.";
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
        std::cout << " ����������� ������� �� ������� ������ : " << i + 1 << std::endl;
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
            std::cout << " ���������� ������� �� ������� ������ : " << item + 1 << std::endl;

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
