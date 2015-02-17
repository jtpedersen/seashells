#ifndef RDSYS_H_
#define RDSYS_H_
#include <array>
#include <cmath>
#include <random>

template<int size>
class RDSys {
public:

    RDSys(double diffuse_a, double base_a, double reduce_a, double diffuse_b, double reduce_b, double base_b) :
	 diffuse_a(diffuse_a), base_a(base_a),  reduce_a(reduce_a),  diffuse_b(diffuse_b), reduce_b(reduce_b),  base_b(base_b) {
	for(auto& ax : a)
	    ax = 0.0;
	for(auto& bx : b)
	    bx = 0.0;
	for(int x = 0; x < size; x++) {
	    a[idx(x, 0)] = 1.0;
	    b[idx(x, 0)] = 1.0;
	}
	static std::random_device rd;
	static std::mt19937 gen(rd());
	const double fluc =.1;
	static std::uniform_real_distribution<> fluctuations(-fluc, fluc);


	for(auto& si: s)
	    si = std::max(0.0, reduce_a + fluctuations(gen));

    }
    void tick(int line);
    
    std::array<double, size*size> a;
    std::array<double, size*size> b;
    std::array<double, size*size> s;

private:
    size_t idx(int x, int y);
    std::array<double, size*size> laplacian(std::array<double, size*size> arr);
    std::array<double, size> d2(std::array<double, size*size> arr, int line);
    std::array<double, size> d2(const std::array<double, size> arr);
    double diffuse_a;
    double base_a;
    double reduce_a;
    double diffuse_b;
    double reduce_b;
    double base_b;
};


template<int size>
void RDSys<size>::tick(int line) {
    // writes values into next line
    // wrap around
    line %=size;
    auto next = (line+1)%size;
    std::array<double, size> da;
    std::array<double, size> db;
    std::array<double, size> tmp_a;
    std::array<double, size> tmp_b;

    for(int x = 1; x < size-1; x++) {
	tmp_a[x] = a[idx(x, line)];
	tmp_b[x] = b[idx(x, line)];
    }
    const int steps = 1000;
    const double delta = 1.0 / double(steps);
    for (int i = 0; i < steps; i++) {
	// fix borders
	tmp_a[0] = a[idx(1, line)];
	tmp_b[0] = b[idx(1, line)];
	tmp_a[size-1] = a[idx(size-2, line)];
	tmp_b[size-1] = b[idx(size-2, line)];

    	const auto d2a = d2(tmp_a);
    	const auto d2b = d2(tmp_b);
    	for(int x = 1; x < size-1; x++) {
    	    const double ax = a[idx(x, line)];
    	    const double bx = b[idx(x, line)];
    	    const double axsq = ax*ax;
    	    da[x] = s[idx(x, line)] * ( (axsq/bx) + base_a) - reduce_a*ax + diffuse_a * d2a[x];
    	    db[x] = s[idx(x, line)] * axsq                  - reduce_b*bx + diffuse_b * d2b[x] + base_b;
    	}
    	for(int x = 0; x < size; x++) {
    	    tmp_a[x] = tmp_a[x] +  delta * da[x];
    	    tmp_b[x] = tmp_b[x] +  delta * db[x];
    	}

    }

    // transfer to newline
    for(int x = 0; x < size; x++) {
	a[idx(x, next)] = std::max(0.0, tmp_a[x]);
	b[idx(x, next)] = std::max(0.0, tmp_b[x]);
    }
}

template<int size>
size_t RDSys<size>::idx(int x, int y) {
    return x + y * size;
}

template<int size>
std::array<double, size> RDSys<size>::d2(std::array<double, size*size> arr, int line) {
    std::array<double, size> res;
    for(int x = 1; x < size-1; x++) {
	res[x] = arr[idx(x-1, line)]
	    -2.0 * arr[idx(x, line)]
	    + arr[idx(x+1, line)];
    }
    return res;
}

template<int size>
std::array<double, size> RDSys<size>::d2(const std::array<double, size> arr) {
    std::array<double, size> res;
//    #pragma omp parallel for
    for(int x = 1; x < size-1; x++) {
	res[x] = arr[x-1]
	    -2.0 * arr[x]
	    + arr[x+1];
    }
    return res;
}

template<int size>
std::array<double, size*size> RDSys<size>::laplacian(std::array<double, size*size> arr) {

    std::array<double, size*size> laplace;
    // first wihtin the bounds
    for (int y = 1; y < size-1; y++) {
	for (int x = 1; x < size-1; x++) {
            auto v = .05 * arr[idx( x-1, y-1)] +
                .2  * arr[idx( x-1, y  )] +
                .05 * arr[idx( x-1, y+1)] +
                .2  * arr[idx( x  , y-1)] +
                -1. * arr[idx( x  , y  )] +
                .2  * arr[idx( x  , y+1)] +
                .05 * arr[idx( x+1, y-1)] +
                .2  * arr[idx( x+1, y  )] +
                .05 * arr[idx( x+1, y+1)];
            laplace[idx(x,y)] = v; 
        }
    }

    return laplace;
}


#endif /* !RDSYS_H_ */
