#ifndef __YUV2RGB_SHADERS_H__
#define __YUV2RGB_SHADERS_H__

#ifdef HAS_GL
#pragma warning( push )
#pragma warning( disable : 4250 )

#include "../../../../guilib/Shader.h"

namespace Shaders {

  class BaseYUV2RGBShader
    : virtual public CShaderProgram
  {
  public:
    virtual ~BaseYUV2RGBShader()      {};
    virtual void SetField(int field)  {};
    virtual void SetWidth(int width)  {};
    virtual void SetHeight(int width) {};

    virtual void SetBlack(float black)       {};
    virtual void SetContrast(float contrast) {};
  };


  class BaseYUV2RGBGLSLShader 
    : public BaseYUV2RGBShader
    , public CGLSLShaderProgram
  {
  public:
    BaseYUV2RGBGLSLShader(bool rect, unsigned flags);
   ~BaseYUV2RGBGLSLShader() {}
    virtual void SetField(int field) { m_field  = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }
    
    virtual void SetBlack(float black)       { m_black    = black; }
    virtual void SetContrast(float contrast) { m_contrast = contrast; }
  protected:
    void OnCompiledAndLinked();
    bool OnEnabled();

    unsigned m_flags;
    int   m_width;
    int   m_height;
    int   m_field;

    float m_black;
    float m_contrast;

    string m_defines;

    // shader attribute handles
    GLint m_hYTex;
    GLint m_hUTex;
    GLint m_hVTex;
    GLint m_hMatrix;
  };

  class BaseYUV2RGBARBShader 
    : public BaseYUV2RGBShader
    , public CARBShaderProgram
  {
  public:
    BaseYUV2RGBARBShader(unsigned flags);
   ~BaseYUV2RGBARBShader() {}
    virtual void SetField(int field) { m_field  = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }
    string       BuildYUVMatrix();

  protected:
    unsigned m_flags;
    int   m_width;
    int   m_height;
    int   m_field;

    // shader attribute handles
    GLint m_hYTex;
    GLint m_hUTex;
    GLint m_hVTex;
  };

  class YUV2RGBProgressiveShaderARB : public BaseYUV2RGBARBShader
  {
  public:
    YUV2RGBProgressiveShaderARB(bool rect=false, unsigned flags=0);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

  class YUV2RGBProgressiveShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBProgressiveShader(bool rect=false, unsigned flags=0);
  };

  class YUV2RGBBobShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBBobShader(bool rect=false, unsigned flags=0);
    void OnCompiledAndLinked();
    bool OnEnabled();

    GLint m_hStepX;
    GLint m_hStepY;
    GLint m_hField;
  };

} // end namespace

#pragma warning( pop )
#endif

#endif //__YUV2RGB_SHADERS_H__
