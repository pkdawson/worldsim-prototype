#pragma once

#include <bitset>
using std::bitset;

// simple matrix class with row-major storage
// fast, no bounds checking

template<typename T>
class Matrix
{
public:
    Matrix()
    {
        _buf = nullptr;
    }

    Matrix(uint32_t x, uint32_t y)
    {
        init(x, y);
    }

    ~Matrix()
    {
        delete[] _buf;
    }

    void init(uint32_t x, uint32_t y)
    {
        _width = x;
        _height = y;
        uint64_t size = static_cast<uint64_t>(_width) * _height;
        _buf = new T[size];
    }

    void fill(T val)
    {
        uint64_t size = static_cast<uint64_t>(_width) * _height;
        uint64_t i;
        for (i = 0; i < size; ++i)
        {
            _buf[i] = val;
        }
    }

    inline T& operator()(uint32_t x, uint32_t y)
    {
        return _buf[y*_width + x];
    }

    inline T operator()(uint32_t x, uint32_t y) const
    {
        return _buf[y*_width + x];
    }

    void scale(Matrix& out, uint32_t factor)
    {
        if (out.getHeight() < _height * factor ||
            out.getWidth() < _width * factor)
            return;

#pragma omp parallel for
        for (int y = 0; y < static_cast<int>(_height); y++) {
            for (size_t x = 0; x < _width; x++) {
                for (size_t oy = 0; oy < factor; oy++) {
                    for (size_t ox = 0; ox < factor; ox++) {
                        out(x*factor + ox, y*factor + oy) = (*this)(x, y);
                    }
                }
            }
        }

    }

    uint32_t getWidth() { return _width; }
    uint32_t getHeight() { return _height; }
    T* data() { return _buf; }
    const T* data() const { return _buf; }

private:
    T* _buf;
    uint32_t _width;
    uint32_t _height;
};

template<uint32_t width, uint32_t height>
class BitsetMatrix
{
public:
    BitsetMatrix()
    {
        _data.reset();
    }

    inline void set(uint32_t x, uint32_t y, bool val)
    {
        _data[y*width + x] = val;
    }

    inline bool operator()(uint32_t x, uint32_t y) const
    {
        return _data[y*width + x];
    }

private:
    bitset<width * height> _data;
};

template<typename T>
class RLEMatrix
{
public:
    RLEMatrix(uint32_t x, uint32_t y)
    {
        init(x, y);
    }

    void init(uint32_t x, uint32_t y, T val=T())
    {
        _width = x;
        _height = y;

        uint32_t zeroes = _width * _height;
        while (zeroes > 0)
        {
            if (zeroes > 65535) {
                _data.emplace_back(val, 65535);
                zeroes -= 65535;
            }
            else {
                _data.emplace_back(val, zeroes);
                zeroes = 0;
            }
        }
    }

    void fill(T val)
    {
        _data.clear();
        init(_width, _height, val);
    }

    inline void set(uint32_t x, uint32_t y, T val)
    {
        uint32_t pos = y * _width + x;
        
        for (size_t idx = 0; idx < _data.size(); ++idx)
        {
            pair<T, uint16_t> next = _data[idx];
            vector<pair<T, uint16_t>> newNodes;
            newNodes.reserve(3);
            if (next.second >= pos)
            {
                // split and insert
                _data.erase(_data.begin() + idx);
                if (pos > 0) {
                    newNodes.emplace_back(next.first, pos - 1);
                }
                newNodes.emplace_back(val, 1);
                if (pos < next.second) {
                    if (pos == 0)
                        newNodes.emplace_back(next.first, next.second - pos - 1);
                    else
                        newNodes.emplace_back(next.first, next.second - pos);
                }
                _data.insert(_data.begin() + idx, newNodes.begin(), newNodes.end());
                // TODO: recompress on either side
                //for (auto& p : _data) {
                //    cout << p.first << ", " << p.second << endl;
                //}
                //cout << endl;

                return;
            }
            pos -= next.second;
        }
    }

    inline T operator()(uint32_t x, uint32_t y) const
    {
        uint32_t pos = y * _width + x;
        for (auto& p : _data)
        {
            if (p.second >= pos)
            {
                return p.first;
            }
            pos -= p.second;
        }
    }

private:
    uint32_t _width;
    uint32_t _height;
    vector<pair<T, uint16_t>> _data;
};
