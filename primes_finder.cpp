#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/version.hpp>
#include <boost/atomic.hpp>
#include <math.h>
#include <vector>
//https://www.boost.org/doc/libs/1_71_0/doc/html/lockfree/examples.html



boost::lockfree::queue<int> GLOBAL_QUEUE(100);
boost::atomic_int NUM_OF_PRIMES(0);


class PrimesPusher {
    int range_from, range_to;

    bool is_prime(int number) {
        if (number < 2) return false;
        for (int i = 2; i <= sqrt(number); ++i) {
            if (number % i == 0) return false;
        }
        return true;
    }
public:

    void push_primes(bool count_primes = true) {
        for (int i = this->range_from; i < this->range_to; ++i) {
            if (is_prime(i)) {
                if (count_primes) ++NUM_OF_PRIMES;
                while (!GLOBAL_QUEUE.push(i));
            }
        }
    }

    PrimesPusher(int from, int to) : range_from(from), range_to(to) {}
    void operator() () {
        this->push_primes();
    }
};


std::vector<PrimesPusher> create_primes_pusher(int thread_count, int block_size) {
    std::vector<PrimesPusher> primes_pusher;
    for (int i = 0; i < thread_count; ++i) {
        int from = i * block_size;
        int to = (i + 1) * block_size;
        primes_pusher.emplace_back(from, to);
        //std::cout << from << "/" << to << std::endl;
    }
    return primes_pusher;
}



//g++ -std=c++20 -O3 -o find_primes primes_finder.cpp -pthread -lboost_thread && ./find_primes
//g++ (Ubuntu 10.3.0-1ubuntu1~20.04) 10.3.0
//sudo apt install libboost-all-dev=1.71.0.0ubuntu2
//maybe you will need to install pthread
int main() {
    
    const int thread_count = 24;
    const int primes_less_than = 10000000; //should be multiple of thread_count for correct behaviour and more than thread count   
    const int block_size = primes_less_than/thread_count;    
    std::cout << "Finding primes less than " << primes_less_than <<
                 " on " << thread_count << " threads" << std::endl;


    auto primes_pusher = create_primes_pusher(thread_count, block_size);


    std::cout << "Starting now" << std::endl;
    auto start = std::chrono::system_clock::now();

    boost::thread_group threads;
    for (auto& range_solver : primes_pusher) {
        threads.create_thread(range_solver);
    }
    threads.join_all();
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout  << "Found " << NUM_OF_PRIMES << " primes in " << elapsed.count() << " miliseconds \n";


    auto functor = [](auto){ return 0; };
    size_t queue_size = GLOBAL_QUEUE.consume_all(functor);

    std::cout << "There were " << queue_size << " objects in queue" << std::endl;

    //std::cout << BOOST_LIB_VERSION << '\n';


    return 0;
}