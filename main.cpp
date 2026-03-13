#include <cstdint>
#include <stdexcept>

template<class type>
class vector {
public:
    vector(uint64_t Size = 0, uint64_t IncreaseVal = 2, bool IncreaseMult = true) {
        size = Size;
        capacity = 8;
        increaseVal = IncreaseVal;
        increaseMult = IncreaseMult;
        while (capacity < Size) increase();
        data = new type[capacity];
    }
    void resize(uint64_t Size, bool brute = false) {
        if (Size > capacity) {
            if (!brute) throw std::out_of_range("Size");
            recapacity(Size);
        }
        size = Size;
    }
    void recapacity(uint64_t Capacity, bool brute = false) {
        if (size > Capacity) {
            if (!brute)  throw std::out_of_range("Size");
            resize(Capacity, 1);
        }
        type* tmp = data;
        data = new type[Capacity];
        for (uint64_t i = 0; i < size && i < Capacity; i++) {
            data[i] = tmp[i];
        }
        delete[] tmp;
    }
private:
    void increase() {
        if (increaseMult) capacity *= increaseVal;
        else capacity += increaseVal;
    }

    type* data;
    uint64_t size;
    uint64_t capacity;

    uint64_t increaseVal;
    bool increaseMult;
};
