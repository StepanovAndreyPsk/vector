#ifndef VECTOR_H_
#define VECTOR_H_

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace lab_07 {

template <typename T, typename Alloc = std::allocator<T>>
class vector {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

    std::size_t v_size = 0;
    std::size_t v_capacity = 0;

    T *data{};

public:
    vector() = default;

    explicit vector(std::size_t n)
        : v_size(n), v_capacity(calc_capacity(n)), data(alloc(v_capacity)) {
        for (std::size_t i = 0; i < n; i++) {
            try {
                new (data + i) T();
            } catch (...) {
                destruct(0, i);
                Alloc().deallocate(data, v_capacity);
                throw;
            }
        }
    }

    vector(std::size_t n, const T &elem)
        : v_size(n), v_capacity(calc_capacity(n)), data(alloc(v_capacity)) {
        for (std::size_t i = 0; i < n; i++) {
            try {
                new (data + i) T(elem);
            } catch (...) {
                destruct(0, i);
                dealloc();
                throw;
            }
        }
    }

    vector(const vector &other)
        : v_size(other.v_size),
          v_capacity(calc_capacity(other.v_size)),
          data(alloc(v_capacity)) {
        for (std::size_t i = 0; i < v_size; i++) {
            try {
                new (data + i) T(other[i]);
            } catch (...) {
                destruct(0, i);
                dealloc();
                throw;
            }
        }
    }

    vector(vector &&other) noexcept
        : v_size(std::exchange(other.v_size, 0)),
          v_capacity(std::exchange(other.v_capacity, 0)),
          data(std::exchange(other.data, nullptr)) {
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return v_size;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return v_capacity;
    }

    [[nodiscard]] bool empty() const noexcept {
        return v_size == 0;
    }

    T &at(std::size_t index) & {
        if (index < v_size) {
            return data[index];
        } else {
            throw std::out_of_range("index out of range");
        }
    }

    const T &at(std::size_t index) const & {
        if (index < v_size) {
            return data[index];
        } else {
            throw std::out_of_range("index out of range");
        }
    }

    T &&at(std::size_t index) && {
        if (index < v_size) {
            return std::move(data[index]);
        } else {
            throw std::out_of_range("index out of range");
        }
    }

    T &operator[](std::size_t index) & {
        return data[index];
    }

    const T &operator[](std::size_t index) const & {
        return data[index];
    }

    T &&operator[](std::size_t index) && {
        return std::move(data[index]);
    }

    vector &operator=(vector &&other) noexcept {
        if (this->data == other.data) {
            return *this;
        }

        destruct(0, v_size);
        v_size = std::exchange(other.v_size, 0);
        std::swap(v_capacity, other.v_capacity);
        std::swap(data, other.data);

        return *this;
    }

    vector &operator=(const vector &other) {
        if (this->data == other.data) {
            return *this;
        }
        auto tmp = vector(other);
        *this = std::move(tmp);
        return *this;
    }

    void push_back(const T &element) & {
        if (v_size == v_capacity) {
            std::size_t updated_cap = calc_capacity(v_size + 1);
            T *buffer = alloc(updated_cap);
            try {
                new (buffer + v_size) T(element);
            } catch (...) {
                dealloc(buffer, updated_cap);
                throw;
            }
            move_elements(data, buffer, v_size);
            dealloc();
            data = buffer;
            v_capacity = updated_cap;
        } else {
            try {
                new (data + v_size) T(element);
            } catch (...) {
                throw;
            }
        }
        v_size++;
    }

    void push_back(T &&element) &noexcept {
        if (v_size == v_capacity) {
            std::size_t updated_cap = calc_capacity(v_size + 1);
            T *buffer = alloc(updated_cap);
            move_elements(data, buffer, v_size);
            dealloc();
            data = buffer;
            v_capacity = updated_cap;
        }
        new (data + v_size) T(std::move(element));
        v_size++;
    }

    void pop_back() &noexcept {
        (data + v_size - 1)->~T();
        v_size--;
    }

    void reserve(std::size_t updated_cap) & {
        if (updated_cap <= v_capacity) {
            return;
        }
        updated_cap = calc_capacity(updated_cap);
        T *new_buffer = alloc(updated_cap);
        move_elements(data, new_buffer, v_size);
        dealloc();
        data = new_buffer;
        v_capacity = updated_cap;
    }

    void clear() &noexcept {
        destruct(0, v_size);
        v_size = 0;
    }

    void resize(std::size_t new_size) & {
        if (new_size == v_size) {
            return;
        }
        if (new_size < v_size) {
            destruct(new_size, v_size);
        } else if (new_size <= v_capacity) {
            for (std::size_t i = v_size; i < new_size; i++) {
                try {
                    new (data + i) T();
                } catch (...) {
                    destruct(v_size, i);
                    throw;
                }
            }
        } else {
            std::size_t updated_cap = calc_capacity(new_size);
            T *buffer = alloc(updated_cap);

            for (std::size_t i = v_size; i < new_size; i++) {
                try {
                    new (buffer + i) T();
                } catch (...) {
                    dealloc(buffer, updated_cap);
                    throw;
                }
            }
            move_elements(data, buffer, v_size);
            dealloc();
            v_capacity = updated_cap;
            data = buffer;
        }
        v_size = new_size;
    }

    void resize(std::size_t new_size, const T &element) & {
        if (new_size == v_size) {
            return;
        }
        if (new_size < v_size) {
            destruct(new_size, v_size);
        } else if (new_size <= v_capacity) {
            for (std::size_t i = v_size; i < new_size; i++) {
                try {
                    new (data + i) T(element);
                } catch (...) {
                    destruct(v_size, i);
                    throw;
                }
            }
        } else {
            std::size_t updated_cap = calc_capacity(new_size);
            T *buffer = alloc(updated_cap);

            for (std::size_t i = v_size; i < new_size; i++) {
                try {
                    new (buffer + i) T(element);
                } catch (...) {
                    dealloc(buffer, updated_cap);
                    throw;
                }
            }
            move_elements(data, buffer, v_size);
            dealloc();
            v_capacity = updated_cap;
            data = buffer;
        }
        v_size = new_size;
    }

    ~vector() noexcept {
        destruct(0, v_size);
        dealloc();
    }

private:
    std::size_t calc_capacity(std::size_t n) {
        if (n == 0) {
            return 0;
        }
        std::size_t c = 1;
        while (c < n) {
            c *= 2;
        }
        return c;
    }

    void destruct(std::size_t from, std::size_t n) {
        for (std::size_t i = from; i < n; i++) {
            (data + i)->~T();
        }
    }

    T *alloc(std::size_t n) {
        return (n == 0) ? nullptr : Alloc().allocate(n);
    }

    void dealloc() {
        if (data != nullptr && v_capacity != 0) {
            Alloc().deallocate(data, v_capacity);
        }
    }

    void dealloc(T *buffer, std::size_t n) {
        if (buffer != nullptr && n != 0) {
            Alloc().deallocate(buffer, n);
        }
    }

    void move_elements(T *from, T *to, std::size_t n) {
        for (std::size_t i = 0; i < n; i++) {
            new (to + i) T(std::move(from[i]));
        }
    }
};

}  // namespace lab_07

#endif  // VECTOR_H_
