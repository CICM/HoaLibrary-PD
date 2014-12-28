/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef DEF_HOA_2D_PD
#define DEF_HOA_2D_PD

#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
#include "../hoa.pd.h"

using namespace hoa;

extern "C" void setup_hoa0x2e2d0x2eencoder_tilde(void);
extern "C" void setup_hoa0x2e2d0x2eoptim_tilde(void);
extern "C" void setup_hoa0x2e2d0x2erotate_tilde(void);
extern "C" void setup_hoa0x2e2d0x2escope_tilde(void);
extern "C" void setup_hoa0x2e2d0x2eprojector_tilde(void);
//extern "C" void setup_hoa0x2e2d0x2erecomposer_tilde(void);
extern "C" void setup_hoa0x2e2d0x2espace(void);
extern "C" void setup_hoa0x2e2d0x2emeter_tilde(void);
/*
 
 extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void);
 */

template <typename T> class PolarLines
{
    
private:
    T*      m_values_old;
    T*      m_values_new;
    T*      m_values_step;
    ulong   m_counter;
    ulong   m_ramp;
    ulong   m_number_of_sources;
    
public:
    PolarLines(ulong numberOfSources) noexcept :
    m_number_of_sources(numberOfSources)
    {
        m_values_old    = new T[m_number_of_sources * 2];
        m_values_new    = new T[m_number_of_sources * 2];
        m_values_step   = new T[m_number_of_sources * 2];
    }
    
    ~PolarLines()
    {
        delete [] m_values_old;
        delete [] m_values_new;
        delete [] m_values_step;
    }
    
    inline ulong getNumberOfSources() const noexcept
    {
        return m_number_of_sources;
    }
    
    inline ulong getRamp() const noexcept
    {
        return m_ramp;
    }
    
    inline double getRadius(ulong index) const noexcept
    {
        return m_values_new[index];
    }
    
    inline double getAzimuth(ulong index) const noexcept
    {
        return m_values_new[m_number_of_sources +index];
    }
    
    inline void setRamp(const ulong ramp) noexcept
    {
        m_ramp = std::max(ramp, (ulong)1);
    }
    
    inline void setRadius(const ulong index, const T radius) noexcept
    {
        m_values_new[index]  = radius;
        m_values_step[index] = (m_values_new[index] - m_values_old[index]) / (double)m_ramp;
        m_counter = 0;
    }
    
    inline void setAzimuth(const ulong index, const T azimuth) noexcept
    {
        m_values_new[index + m_number_of_sources] = wrap_twopi(azimuth);
        m_values_old[index + m_number_of_sources] = wrap_twopi(m_values_old[index + m_number_of_sources]);
        
        double distance;
        if(m_values_old[index + m_number_of_sources] > m_values_new[index + m_number_of_sources])
            distance = (m_values_old[index + m_number_of_sources] - m_values_new[index + m_number_of_sources]);
        else
            distance = (m_values_new[index + m_number_of_sources] - m_values_old[index + m_number_of_sources]);
        
        if(distance <= HOA_PI)
        {
            m_values_step[index + m_number_of_sources] = (m_values_new[index + m_number_of_sources] - m_values_old[index + m_number_of_sources]) / (double)m_ramp;
        }
        else
        {
            if(m_values_new[index + m_number_of_sources] > m_values_old[index + m_number_of_sources])
            {
                m_values_step[index + m_number_of_sources] = ((m_values_new[index + m_number_of_sources] - HOA_2PI) - m_values_old[index + m_number_of_sources]) / (double)m_ramp;
            }
            else
            {
                m_values_step[index + m_number_of_sources] = ((m_values_new[index + m_number_of_sources] + HOA_2PI) - m_values_old[index + m_number_of_sources]) / (double)m_ramp;
            }
        }
        m_counter = 0;
    }
    
    inline void setRadiusDirect(const ulong index, const T radius) noexcept
    {
        m_values_old[index] = m_values_new[index] = radius;
        m_values_step[index] = 0.;
        m_counter = 0;
    }
    
    inline void setAzimuthDirect(const ulong index, const T azimuth) noexcept
    {
        m_values_old[index + m_number_of_sources] = m_values_new[index + m_number_of_sources] = azimuth;
        m_values_step[index + m_number_of_sources] = 0.;
        m_counter = 0;
    }
    
    inline void process(float* vector) noexcept
    {
        cblas_saxpy(m_number_of_sources * 2, 1., m_values_step, 1, m_values_old, 1);
        if(m_counter++ >= m_ramp)
        {
            cblas_scopy(m_number_of_sources * 2, m_values_new, 1, m_values_old, 1);
            memset(m_values_step, 0, sizeof(float) * m_number_of_sources * 2);
            m_counter    = 0;
        }
        cblas_scopy(m_number_of_sources * 2, m_values_old, 1, vector, 1);
    }
    
    inline void process(double* vector) noexcept
    {
        cblas_daxpy(m_number_of_sources * 2, 1., m_values_step, 1, m_values_old, 1);
        if(m_counter++ >= m_ramp)
        {
            cblas_scopy(m_number_of_sources * 2, m_values_new, 1, m_values_old, 1);
            memset(m_values_step, 0, sizeof(float) * m_number_of_sources * 2);
            m_counter    = 0;
        }
        cblas_dcopy(m_number_of_sources * 2, m_values_old, 1, vector, 1);
    }
};


//! The ambisonic scope.
/** The scope discretize a circle by a set of point and uses a decoder to project the circular harmonics on it. This class should be used for graphical interfaces outside the digital signal processing if the number of points to discretize the circle is very large. Then you should prefer to record snapshot of the circular harmonics and to call the process method at an interval adapted to a graphical rendering.
 */
template <typename T> class Scope : public Decoder<Hoa2d, T>
{
private:
    T   m_maximum;
    T*  m_matrix;
public:
    
    //! The scope constructor.
    /**	The scope constructor allocates and initialize the member values to computes circular harmonics projection on a circle depending on a decomposition order and a circle discretization. The circle is discretized by the number of points. The order must be at least 1. The number of points and column should be at least 3 (but it's very low).
     
     @param     order            The order.
     @param     numberOfPoints   The number of points.
     */
    Scope(unsigned long order, unsigned long numberOfPoints) : Decoder<Hoa2d, T>(order, numberOfPoints)
    {
        m_matrix = new T[Decoder<Hoa2d, T>::getNumberOfChannels()];
        for(unsigned long i = 0; i < Decoder<Hoa2d, T>::getNumberOfChannels(); i++)
        {
            m_matrix[i] = 0.;
        }
        m_maximum = 0;
    }
    
    //! The Scope destructor.
    /**	The Scope destructor free the memory.
     */
    ~Scope()
    {
        delete [] m_matrix;
    }
    
    //! Retrieve the number of points.
    /**	Retrieve the number of points used to discretize the ambisonic circle.
     @return     This method returns the number of points used to discretize the circle.
     */
    inline unsigned long getNumberOfPoints() const noexcept
    {
        return Decoder<Hoa2d, T>::getNumberOfChannels();
    }
    
    //! Retrieve the value of a point of the circular harmonics projection.
    /**	Retrieve the result value of the circular harmonics projection for a given point defined by an index. The absolute of the value can be used as the radius of the point for a 2 dimentionnal representation. For the index, 0 is the 0 azimtuh of the circle. The maximum index must be the number of points - 1.
     @param     index   The point index of the point.
     @return    This method returns the value of a point of the ambisonic circle.
     */
    inline T getPointValue(const unsigned long index) const noexcept
    {
        return m_matrix[index];
    }
    
    //! Retrieve the radius of a point of the circular harmonics projection.
    /**	Retrieve the radius of the circular harmonics projection for a given point defined by an index. This the absolute of the result of the projection. For the index, 0 is the 0 azimtuh of the circle. The maximum index must be the number of points - 1.
     @param     pointIndex   The point index of the point.
     @return    This method returns the radius of a point of the ambisonic circle.
     */
    inline T getPointRadius(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]);
    }
    
    //! Retrieve the azimuth of a point of the circular harmonics projection.
    /**	Retrieve the azimuth of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     @param     pointIndex   The point index of the point.
     @return    This method returns the azimuth of a point of the ambisonic circle.
     */
    inline T getPointAzimuth(const unsigned long index) const noexcept
    {
        return Decoder<Hoa2d, T>::getChannelAzimuth(index);
    }
    
    //! Retrieve the abscissa of a point of the circular harmonics projection.
    /**	Retrieve the abscissa of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     
     @param     pointIndex   The point index of the point.
     @return    This method returns the abscissa of a point of the ambisonic circle.
     
     @see       getOrdinate
     */
    inline T getPointAbscissa(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]) * Decoder<Hoa2d, T>::getChannelAbscissa(index);
    }
    
    //! Retrieve the ordinate of a point of the circular harmonics projection.
    /**	Retrieve the ordinate of the circular harmonics projection for a given point defined by an index.The maximum index must be the number of points - 1.
     
     @param     pointIndex   The point index of the point.
     @return    This method returns the ordinate of a point of the ambisonic circle.
     
     @see       getAbscissa
     */
    inline T getPointOrdinate(const unsigned long index) const noexcept
    {
        return fabs(m_matrix[index]) * Decoder<Hoa2d, T>::getChannelOrdinate(index);
    }
    
    //! This method performs the circular harmonics projection.
    /**	You should use this method to compute the projection of the circular harmonics over an ambisonics circle. The inputs array contains the circular harmonics samples and the minimum size must be the number of harmonics.
     @param     inputs   The inputs array.
     */
    inline void process(const T* inputs) noexcept
    {
        Decoder<Hoa2d, T>::process(inputs, m_matrix);
        m_maximum = fabsf(vector_max(Decoder<Hoa2d, T>::getNumberOfChannels(), m_matrix));
        if(m_maximum > 1.)
        {
            vector_scale(Decoder<Hoa2d, T>::getNumberOfChannels(), (1. / m_maximum), m_matrix);
        }
    }
};
#endif
