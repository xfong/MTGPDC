#ifndef SHORTEST_BASIS_HPP
#define SHORTEST_BASIS_HPP 1
/**
 * @file shortest_basis.hpp
 *
 * @brief calculate equidistribution property of the random number generator.
 *
 * calculate shortest basis of lattice. We can get the dimension of
 * equidistribution from the smallest norm of the vectors in lattice
 * basis.
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (c) 2010 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 * Copyright (c) 2011 Mutsuo Saito, Makoto Matsumoto, Hiroshima
 * University and University of Tokyo. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */
#include <tr1/memory>
#include <stdexcept>
#include <NTL/GF2.h>

namespace MTToolBox {
    /**
     * @class linear generator as a vector
     *
     * @tparam G F2-linear generator
     * @tparam T the output type of \b G.
     */
    template<typename G, typename T> class linear_generator_vector {
    public:
	/**
	 * constructor
	 *
	 * @param rand_ random number generator
	 */
	linear_generator_vector<G, T>(const G& rand_) {
	    rand = rand_.clone();
	    rand->init_zero_ex();
	    count = 0;
	    zero = false;
	    next = 0;
	};

	/**
	 * constructor of a vector in standard basis
	 *
	 * @param rand_ random number generator used as a template
	 * @param bit_pos position of 1 for standard basis.
	 */
	linear_generator_vector<G, T>(const G& rand_, int bit_pos) {
	    rand = rand_.zero();
	    count = 0;
	    zero = false;
	    next = static_cast<T>(1) << (bit_size(T) - bit_pos - 1);
	};

	void add(const linear_generator_vector<G, T>& src);
	void next_state(int bit_len);
	void debug_print();

	/**
	 * linear generator with internal state
	 */
	std::tr1::shared_ptr<G> rand;

	/**
	 * The counter which shows how many times next_state() is called.
	 * This is concerned with norm of vector and degree of polynomial.
	 */
	int count;

	/**
	 * This shows the vector is zero or not.
	 */
	bool zero;

	/**
	 * This is important.
	 * v-bit MSBs of generator's recent output.
	 * Or, the coefficent of highest degree term of polynomial.
	 */
	T next;
    };

    /**
     * @class shotest_basis
     * @brief calculate shortest basis of lattice
     *
     * @tparam G generator
     * @tparam T output type of the generator
     */
    template<typename G, typename T> class shortest_basis {
	typedef linear_generator_vector<G, T> linear_vec;
    public:
	/**
	 * constructor
	 *
	 * @param rand linear generator
	 * @param bit_len_ from \b bit_len to 1 of dimension of
	 * equidistribution with v-bit accuracy
	 */
	shortest_basis(const G& rand, int bit_len_) {
	    bit_len = bit_len_;
	    size = bit_len + 1;
	    basis = new linear_vec * [size];
	    mexp = rand.get_mexp();
	    for (int i = 0; i < bit_len; i++) {
		basis[i] = new linear_vec(rand, i);
	    }
	    basis[bit_len] = new linear_vec(rand);
	    basis[bit_len]->next_state(bit_len);
	};
	~shortest_basis() {
	    for (int i = 0; i < size; i++) {
		delete basis[i];
	    }
	    delete[] basis;
	};

	int get_all_equidist(int veq[]);
	int get_equidist(int *sum_equidist);
    private:
	int get_equidist_main(int bit_len);
	void adjust(int new_len);
	/** basis of lattice plus one vector */
	linear_vec **basis;
	/** bit lenght count from MSB */
	int bit_len;
	/** Mersenne Exponent */
	int mexp;
	int size;
    };

    /**
     * adjust bit_len and recalculate the coefficient of the highest
     * degree term.
     */
    template<typename G, typename T>
    void shortest_basis<G, T>::adjust(int new_len) {
	using namespace std;

	T mask = (~static_cast<T>(0)) << (bit_size(T) - new_len);
#if 0
	if (basis[basis.size() - 1]->zero) {
	    basis.erase(basis.begin() + basis.size() - 1);
	} else {
	    cerr << "no zero state" << endl;
	    throw new logic_error("no zero state");
	}
#endif
	for (int i = 0; i < size; i++) {
	    basis[i]->next = basis[i]->next & mask;
	    if (basis[i]->next == 0) {
		basis[i]->next_state(new_len);
	    }
	}
    }

    /**
     * print some information
     */
    template<typename G, typename T>
    void linear_generator_vector<G, T>::debug_print() {
	using namespace std;

	cout << "debug ====" << endl;
	cout << "count = " << dec << count << endl;
	cout << "zero = " << zero << endl;
	cout << "next = " << hex << next << endl;
	cout << "debug ====" << endl;
	//rand->debug_print();
    }

    /**
     * calculate the dimension of equidistribution with v-bit
     * accuracy, where v is form 1 to \b bit_len
     *
     * @param[out] veq dimension of equidistribution at v
     * @return sum of the diffrence between the theoretical
     * upper bound and veq.
     */
    template<typename G, typename T>
    int shortest_basis<G, T>::get_all_equidist(int veq[]) {
	using namespace std;

	int sum = 0;

	veq[bit_len - 1] = get_equidist_main(bit_len);
#ifdef DEBUG
	for (int i = 0; i < size; i++) {
	    basis[i]->debug_print();
	}
#endif
	sum += mexp / bit_len - veq[bit_len - 1];
	bit_len--;
	for (; bit_len >= 1; bit_len--) {
	    adjust(bit_len);
	    veq[bit_len - 1] = get_equidist_main(bit_len);
	    sum += mexp / bit_len - veq[bit_len - 1];
	}
	return sum;
    }

    /**
     * calculate the dimension of equidistribution with \b bit_len
     * accuracy, and additionally sum of the difference between the
     * theoretical upper bound and veq from veq is 1 to \b bit_len -1
     *
     * @param sum_equidist sum of the diffrence
     * @return the dimension of equidistribution at \b bit_len
     */
    template<typename G, typename T>
    int shortest_basis<G, T>::get_equidist(int *sum_equidist) {
	using namespace std;

	int veq = get_equidist_main(bit_len);
	int sum = 0;
	bit_len--;
	for (; bit_len >= 1; bit_len--) {
	    adjust(bit_len);
	    sum += mexp / bit_len - get_equidist_main(bit_len);
	}
	*sum_equidist = sum;
	return veq;
    }

    /**
     * addition of F2((t)) vectors
     * @param src source of addition
     */
    template<typename G, typename T>
    void linear_generator_vector<G, T>::add(
	const linear_generator_vector<G, T>& src) {
	using namespace std;

	rand->add(*src.rand);
	next ^= src.next;
    }

    /**
     * transfer to the next state or nth next state so that the
     * coeffcient of the maximum degree term is non-zero. If internal
     * state is all zero, then set zero flag.
     *
     * @param bit_len bit length from MSB
     */
    template<typename G, typename T>
    void linear_generator_vector<G, T>::next_state(int bit_len) {
	using namespace std;

	if (zero) {
	    return;
	}
	int zero_count = 0;
	next = rand->generate(bit_len);
	count++;
	while (next == 0) {
	    zero_count++;
	    if (zero_count > rand->get_mexp() * 2) {
		zero = true;
		if (rand->is_zero()) {
		    zero = true;
		}
		break;
	    }
	    next = rand->generate(bit_len);
	    count++;
	}
    }

    /**
     * calculate dimension of equidistirbution with v bit accuracy
     * for one v.
     *
     * @param bit_len bit length from MSB, so bit_Len is v.
     */
    template<typename G, typename T>
    int shortest_basis<G, T>::get_equidist_main(int bit_len) {
	using namespace std;
	using namespace NTL;

	int pivot_index;
	int old_pivot = 0;

	pivot_index = calc_1pos(basis[bit_len]->next);
	while (!basis[bit_len]->zero) {
#ifdef DEBUG
	    if (pivot_index != calc_1pos(basis[pivot_index]->next)) {
		cerr << "pivot error 1" << endl;
		cerr << "pivot_index:" << dec << pivot_index << endl;
		cerr << "calc_1pos:" << dec
		     << calc_1pos(basis[pivot_index]->next) << endl;
		cerr << "next:" << hex << basis[pivot_index]->next << endl;
		throw new std::logic_error("pivot error 1");
	    }
#endif
	    if (basis[bit_len]->count > basis[pivot_index]->count) {
		swap(basis[bit_len], basis[pivot_index]);
	    }
	    basis[bit_len]->add(*basis[pivot_index]);
	    if (basis[bit_len]->next == 0) {
		basis[bit_len]->next_state(bit_len);
		pivot_index = calc_1pos(basis[bit_len]->next);
	    } else {
		old_pivot = pivot_index;
		pivot_index = calc_1pos(basis[bit_len]->next);
		if (old_pivot <= pivot_index) {
		    cerr << "pivot error 2" << endl;
		    throw new std::logic_error("pivot error 2");
		}
	    }
	}
	int min_count = basis[0]->count;
	for (int i = 1; i < bit_len; i++) {
	    if (min_count > basis[i]->count) {
		min_count = basis[i]->count;
	    }
	}
	if (min_count > mexp / bit_len) {
	    cout << "over theoretical bound " << bit_len << endl;
	    for(int i = 0; i < size; i++) {
		basis[i]->debug_print();
	    }
	    throw new std::logic_error("over theoretical bound");
	}
	return min_count;
    }
}
#endif
