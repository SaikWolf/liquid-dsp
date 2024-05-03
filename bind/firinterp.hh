// firinterp c++/python bindings
#ifndef __FIRINTERP_HH__
#define __FIRINTERP_HH__

#include <complex>
#include <iostream>
#include <string>
#include "liquid.hh"
#include "liquid.python.hh"

namespace liquid {

class firinterp : public object
{
  public:
    // external coefficients
    firinterp(unsigned int _M, float * _h, unsigned int _n)
        { q = firinterp_crcf_create(_M, _h, _n); }

    // default: kaiser low-pass
    firinterp(unsigned int _M, unsigned int _m=12, float _As=60.0f)
        { q = firinterp_crcf_create_kaiser(_M, _m, _As); }

    // rnyquist
    firinterp(int _ftype, unsigned int _M, unsigned int _m=7, float _beta=0.25f, float _mu=0.0f)
        {  q = firinterp_crcf_create_prototype(_ftype, _M, _m, _beta, _mu); }

    // copy constructor
    firinterp(const firinterp &m) { q = firinterp_crcf_copy(m.q); }

    // destructor
    ~firinterp() { firinterp_crcf_destroy(q); }

    // reset object
    void reset() { firinterp_crcf_reset(q); }

    // object type
    std::string type() const { return "firinterp"; }

    // representation
    std::string repr() const { return std::string("<liquid.interp") +
                    ", M=" + std::to_string(get_interp_rate()) +
                    ", sub-len=" + std::to_string(get_sub_len()) +
                    ">"; }

    // print
    void display() { firinterp_crcf_print(q); }

    // output scale
    void set_scale(float _scale) { firinterp_crcf_set_scale(q, _scale); }
    float get_scale() const { float s; firinterp_crcf_get_scale(q, &s); return s; }

    // get interpolation rate
    unsigned int get_interp_rate() const { return firinterp_crcf_get_interp_rate(q); }

    // get sub-filter length
    unsigned int get_sub_len() const { return firinterp_crcf_get_sub_len(q); }

    // execute on a single sample
    void execute(std::complex<float> _v, std::complex<float> * _buf)
        { firinterp_crcf_execute(q, _v, _buf); }

    // execute on block of samples
    void execute(std::complex<float> * _x,
                 unsigned int          _n,
                 std::complex<float> * _y)
    { firinterp_crcf_execute_block(q, _x, _n, _y); }

  private:
    firinterp_crcf q;

#ifdef LIQUID_PYTHONLIB
  public:
    // python-specific constructor with keyword arguments
    firinterp(std::string ftype, py::kwargs o) {
        auto lupdate = [](py::dict o, py::dict d) {auto r(d);for (auto p: o){r[p.first]=p.second;} return r;};
        int  prototype = liquid_getopt_str2firfilt(ftype.c_str());
        if (prototype != LIQUID_FIRFILT_UNKNOWN) {
            auto v = lupdate(o, py::dict("k"_a=2, "m"_a=5, "beta"_a=0.2f, "mu"_a=0.0f));
            q = firinterp_crcf_create_prototype(prototype,
                py::int_(v["k"]), py::int_(v["m"]), py::float_(v["beta"]), py::float_(v["mu"]));
        } else if (ftype == "lowpass") {
            auto v = lupdate(o, py::dict("n"_a=21, "fc"_a=0.25f, "As"_a=60.0f, "mu"_a=0.0f));
            q = firinterp_crcf_create_kaiser(
                py::int_(v["n"]), py::int_(v["m"]), py::float_(v["As"]));
        } else {
            throw std::runtime_error("invalid/unsupported filter type: " + ftype);
        }
    }

#if 0
    // external coefficients using numpy array
    firinterp(py::array_t<float> _h) {
        // get buffer info and verify parameters
        py::buffer_info info = _h.request();
        if (info.itemsize != sizeof(float))
            throw std::runtime_error("invalid input numpy size, use dtype=np.single");
        if (info.ndim != 1)
            throw std::runtime_error("invalid number of input dimensions, must be 1-D array");

        // create object
        q = firinterp_crcf_create((float*)info.ptr, info.shape[0]);
    }
#endif

    /// run on single value
    py::array_t<std::complex<float>> py_execute_one(std::complex<float> _v)
    {
        // allocate output buffer and execute on data sample
        py::array_t<std::complex<float>> buf_out(get_interp_rate());
        execute(_v, (std::complex<float>*) buf_out.request().ptr);
        return buf_out;
    }

    /// run on array of values
    py::array_t<std::complex<float>> py_execute(py::object & _v)
    {
        // TODO: clean up type inference here
        if ( py::isinstance< py::array_t<std::complex<float>> >(_v) ) {
            // pass
        } else if ( py::isinstance< py::array_t<std::complex<double>> >(_v) ) {
            // try type-casting to array of std::complex<float>
            py::detail::type_caster<py::array_t<std::complex<float>>> tc;
            if (tc.load(_v,true))
                return py_execute(*tc);
            throw std::runtime_error("invalid/unknown input type");
        } else {
            // try type-casting to std::complex<float>
            py::detail::type_caster<std::complex<float>> tc;
            if (tc.load(_v,true))
                return py_execute_one(*tc);
            throw std::runtime_error("invalid/unknown input type");
        }

        // get buffer info
        py::array_t<std::complex<float>> _buf = py::cast<py::array_t<std::complex<float>>>(_v);
        py::buffer_info info = _buf.request();

        // verify input size and dimensions
        if (info.itemsize != sizeof(std::complex<float>))
            throw std::runtime_error("invalid input numpy size, use dtype=np.csingle");
        if (info.ndim != 1)
            throw std::runtime_error("invalid number of input dimensions, must be 1-D array");

        // comptue sample size, number of samples in buffer, and stride between samples
        size_t       ss = sizeof(std::complex<float>);
        unsigned int n0 = info.shape[0];
        if (info.strides[0]/ss != 1)
            throw std::runtime_error("invalid input stride, must be 1");

        // allocate output buffer
        unsigned int num_output = n0*firinterp_crcf_get_interp_rate(q);
        py::array_t<std::complex<float>> buf_out(num_output);

        // execute on data samples
        execute((std::complex<float>*) info.ptr, n0,
                (std::complex<float>*) buf_out.request().ptr);
        return buf_out;
    }
#endif
};

#ifdef LIQUID_PYTHONLIB
static void init_firinterp(py::module &m)
{
    py::class_<firinterp>(m, "firinterp",
        "Finite impulse response interpolator")
        //.def(py::init<std::string, py::kwargs>())
        .def(py::init<unsigned int, unsigned int, float>(),
             py::arg("M"), py::arg("m")=12, py::arg("As")=60.,
             "create default interpolator given rate")
        .def(py::init<std::string,
            py::kwargs>(),
            "create filter from prototype")
        .def("__repr__", &firinterp::repr)
        .def("reset",      &firinterp::reset,      "reset object's internal state")
        .def("execute",    &firinterp::py_execute, "execute on a block of samples")
        .def_property_readonly("rate",
            &firinterp::get_interp_rate,
            "get interpolation rate")
        .def_property_readonly("semilength",
            &firinterp::get_sub_len,
            "get interpolator semi-rate")
        .def_property("scale",
            &firinterp::get_scale,
            &firinterp::set_scale,
            "get/set filter output scaling property")
        ;
}
#endif

} // namespace liquid

#endif //__FIRINTERP_HH__
