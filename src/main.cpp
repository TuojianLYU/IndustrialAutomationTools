#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

using namespace std;

queue<int> orders;
mutex mtx;
condition_variable cv;
bool done = false;

void barista(int numOrders) {
  for (int i = 0; i < numOrders; i++) {
    {
      lock_guard<std::mutex> lg(mtx);
      orders.push(i);
      cv.notify_one();
      cout << "Barista: Prepared order " << i << endl;
    }
    this_thread::sleep_for(chrono::milliseconds(1));
  }
}

void waiter(int numOrders) {
  for (int i = 0; i < numOrders; i++) {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, []() { return !orders.empty(); });
    int order = orders.front();
    orders.pop();
    cout << "Waiter: Serving order " << order << endl;
  }
}

int main() {

  int numOrders = 5;

  thread t1(barista, numOrders);

  thread t2(waiter, numOrders);

  t1.join();
  t2.join();

  return 0;
}