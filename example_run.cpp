#include <unistd.h>
#include <math.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <vector>
#include <cyclicbarrier.hpp>

class prime_checker : public cbar::callable {
public:
    prime_checker(uint32_t waitfor) {
        this->waitfor = waitfor;
    }
    virtual ~prime_checker(){
    }
    virtual void run() override {
        std::cout << "Computation started" << std::endl;
    }
    void add_prime(uint32_t prime) {
        std::lock_guard<std::mutex> lck(lock);
        primes.push_back(prime);
    }
    static bool is_prime(uint32_t number) {
        if (number <= 1 ) {
            return false;
        }
        if (number <= 3) {
            return true;
        }
        auto end = (uint32_t)sqrt(number);
        uint32_t i = 2;
        while (i <= end) {
            if (number % i == 0) {
                return false;
            }
            if (i == 2) {
                i++;
            } else {
                i += 2;
            }
        }
        return true;
    }

    void iam_done() {
        std::lock_guard<std::mutex> lck(lock);
        completed++;
        if (completed != waitfor) {
            return;
        }
        auto it = primes.begin();
        while (it != primes.end()){
            std::cout << *it << " is prime " << std::endl;
            ++it;
        }
    }

private:
    std::mutex lock;
    std::vector<int> primes; 
    uint32_t waitfor = 10;
    uint32_t completed = 0;
};

void primecompute(cbar::cyclicbarrier *cb, prime_checker *pc, uint32_t end,
        std::atomic<uint32_t>* start) {
    cb->await(100000000000);
    while (true) {
        auto cur = start->fetch_add(1);
        if (cur > end) break;
        if (pc->is_prime(cur)){
            pc->add_prime(cur);
        }
    }
    pc->iam_done();
    return;
}

int main() {
    auto pc = new prime_checker(10);
    auto c = new cbar::cyclicbarrier(10, pc);
    auto start = new std::atomic<uint32_t>();
    int i = 0;
    std::vector<std::thread*> ts;
    while (i++ < 10) {
        std::thread *t  = new std::thread(primecompute, c, pc, 10000, start);
        ts.push_back(t);
    }
    i = 0;
    while (i < 10) {
        ts[i++]->join();
    }
    i = 0;
    while (i < 10) {
        delete ts[i++];
    }
    delete pc;
    delete c;
    delete start;
    return 0;
}
