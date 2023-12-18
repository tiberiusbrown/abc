#pragma once

#include <cmath>

// second order control system

namespace detail
{

template<class T>
class ctrl2o_base
{
    T alpha, beta;
    T base_dt, rem_dt;
    T y, dydt;

public:
    explicit ctrl2o_base(T omega, T zeta = T(1), T base_dt = T(1) / 1000)
        : alpha(omega * omega)
        , beta(T(2) * omega * zeta)
        , base_dt(base_dt)
        , rem_dt(0)
        , y(0)
        , dydt(0)
    {
    }
    void set(T y_)
    {
        y = y_;
    }
    void set(T y_, T dydt_)
    {
        y = y_;
        dydt = dydt_;
    }
    void advance(T x, T dt)
    {
        rem_dt += dt;
        while(rem_dt >= 0)
        {
            T d2ydt = alpha * (x - y) - beta * dydt;
            y += dydt * base_dt;
            dydt += d2ydt * base_dt;
            rem_dt -= base_dt;
        }
    }
    void advance_radians(T x, T dt)
    {
        rem_dt += dt;
        x = mod2pi(x);
        while(rem_dt >= 0)
        {
            T diff = x - y;
            if(diff > T(+PI))
                diff -= T(PI2);
            if(diff < T(-PI))
                diff += T(PI2);
            T d2ydt = alpha * diff - beta * dydt;
            y += dydt * base_dt;
            dydt += d2ydt * base_dt;
            rem_dt -= base_dt;
            y = mod2pi(y);
        }
    }
    T get() const { return y; }
};

}

using ctrl2o = detail::ctrl2o_base<float>;
using ctrl2od = detail::ctrl2o_base<double>;
