/* Mednafen - Multi-system Emulator
 *
 * Copyright notice for this file:
 *  Scale2x GLslang shader - ported by Pete Bernert
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "main.h"
#include "opengl.h"
#include "shader.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#define MDFN_GL_TRY(x, ...) { x; GLenum errcode = oblt->p_glGetError(); if(errcode != GL_NO_ERROR) { __VA_ARGS__; throw(MDFN_Error(0, _("OpenGL Error: %d\n"), (int)(long long)errcode)); /* FIXME: Throw an error string and not an arcane number. */ } }

static const char *vertexProg = "void main(void)\n{\ngl_Position = ftransform();\ngl_TexCoord[0] = gl_MultiTexCoord0;\n}";

static std::string MakeProgIpolate(unsigned ipolate_axis)	// X & 1, Y & 2, sharp & 4
{
 std::string ret;

 ret = std::string("\n\
	uniform sampler2D Tex0;\n\
	uniform vec2 TexSize;\n\
	uniform vec2 TexSizeInverse;\n\
	uniform float XSharp;\n\
	uniform float YSharp;\n\
	void main(void)\n\
	{\n\
	        vec2 texelIndex = vec2(gl_TexCoord[0]) * TexSize - float(0.5);\n\
	        vec2 texelFract = fract(texelIndex);\n\
	        texelIndex -= texelFract;\n\
		texelIndex += float(0.5);\n\
	        texelIndex *= TexSizeInverse;\n\
	        vec3 texel[4];\n\
		texel[0] = texture2D(Tex0, texelIndex).rgb;\n\
	        texel[1] = texture2D(Tex0, texelIndex + vec2(0, TexSizeInverse.t)).rgb;\n\
	        texel[2] = texture2D(Tex0, texelIndex + TexSizeInverse).rgb;\n\
	        texel[3] = texture2D(Tex0, texelIndex + vec2(TexSizeInverse.s, 0)).rgb;\n\
	");

 switch(ipolate_axis & 3)
 {
  case 0:
	ret += std::string("gl_FragColor = texture2D(Tex0, vec2(gl_TexCoord[0]));\n");
	break;

  case 1:
	if(ipolate_axis & 4)
	{
	 ret += std::string("\n\
	        float w0 = (texelFract.s - float(0.5)) * XSharp + float(0.5);\n\
	        w0 = clamp(w0, float(0), float(1.0));\n\
		gl_FragColor = vec4( (texel[0] * (1.0 - w0) + texel[3] * w0), 1.0);\n\
		");
	}
	else
	{
	 ret += std::string("\n\
		gl_FragColor = vec4( (texel[0] * (1.0 - texelFract.s) + texel[3] * texelFract.s), 1.0);\n\
		");
	}
	break;

  case 2:
	if(ipolate_axis & 4)
	{
	 ret += std::string("\n\
	        float w1 = (texelFract.t - float(0.5)) * YSharp + float(0.5);\n\
		w1 = clamp(w1, float(0), float(1.0));\n\
		gl_FragColor = vec4( texel[0] * (1.0 - w1) + texel[1] * w1, 1.0);\n\
		");
	}
	else
	{
	 ret += std::string("\n\
		gl_FragColor = vec4( texel[0] * (1.0 - texelFract.t) + texel[1] * texelFract.t, 1.0);\n\
		");
	}
	break;

  case 3:
	if(ipolate_axis & 4)
	{
	 ret += std::string("\n\
	        float w0 = (texelFract.s - float(0.5)) * XSharp + float(0.5);\n\
	        float w1 = (texelFract.t - float(0.5)) * YSharp + float(0.5);\n\
	        w0 = clamp(w0, float(0), float(1.0));\n\
		w1 = clamp(w1, float(0), float(1.0));\n\
		");
	}
	else
	{
	 ret += std::string("\n\
	        float w0 = texelFract.s;\n\
	        float w1 = texelFract.t;\n\
		");
	}

	ret += std::string("\n\
        	gl_FragColor = vec4( (texel[0] * (1.0 - w0) + texel[3] * w0) * (1.0 - w1) + (texel[1] * (1.0 - w0) + texel[2] * w0) * w1, 1);\n\
	       ");
	break;

 }

 ret += std::string("}");

 //if(ipolate_axis == 2)
 // puts(ret.c_str());

 return ret;
}

static const char *fragScale2X =
"uniform vec2 TexSize;\n\
uniform vec2 TexSizeInverse;\n\
uniform sampler2D Tex0;\n\
void main()\n\
{\n\
 vec4 colD,colF,colB,colH,col,tmp;\n\
 vec2 sel;\n\
 vec4 chewx = vec4(TexSizeInverse.x, 0, 0, 0);\n\
 vec4 chewy = vec4(0, TexSizeInverse.y, 0, 0);\n\
 vec4 MeowCoord = gl_TexCoord[0];\n\
 col  = texture2DProj(Tex0, MeowCoord);	\n\
 colD = texture2DProj(Tex0, MeowCoord - chewx);	\n\
 colF = texture2DProj(Tex0, MeowCoord + chewx);	\n\
 colB = texture2DProj(Tex0, MeowCoord - chewy);	\n\
 colH = texture2DProj(Tex0, MeowCoord + chewy);	\n\
 sel=fract(gl_TexCoord[0].xy * TexSize.xy);		\n\
 if(sel.y>=0.5)  {tmp=colB;colB=colH;colH=tmp;}		\n\
 if(sel.x>=0.5)  {tmp=colF;colF=colD;colD=tmp;}		\n\
 if(colB == colD && colB != colF && colD!=colH) 	\n\
  col=colD;\n\
 gl_FragColor = col;\n\
}";

#include "shader_sabr.inc"

void OpenGL_Blitter_Shader::SLP(GLhandleARB moe)
{
 char buf[1000];
 GLsizei buflen = 0;

 oblt->p_glGetInfoLogARB(moe, 999, &buflen, buf);
 buf[buflen] = 0;

 if(buflen)
 {
  throw MDFN_Error(0, "Shader compilation error:\n%s\n", buf);
 }
}

void OpenGL_Blitter_Shader::CompileShader(CompiledShader &s, const char *vertex_prog, const char *frag_prog)
{
	 int opi;

         oblt->p_glEnable(GL_FRAGMENT_PROGRAM_ARB);

         MDFN_GL_TRY(s.v = oblt->p_glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
	 s.v_valid = true;

         MDFN_GL_TRY(s.f = oblt->p_glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
	 s.f_valid = true;

         MDFN_GL_TRY(oblt->p_glShaderSourceARB(s.v, 1, &vertex_prog, NULL), SLP(s.v));
         MDFN_GL_TRY(oblt->p_glShaderSourceARB(s.f, 1, &frag_prog, NULL), SLP(s.f));

         MDFN_GL_TRY(oblt->p_glCompileShaderARB(s.v), SLP(s.v));
	 MDFN_GL_TRY(oblt->p_glGetObjectParameterivARB(s.v, GL_OBJECT_COMPILE_STATUS_ARB, &opi));
	 if(GL_FALSE == opi)
	 {
	  SLP(s.v);
	 }

         MDFN_GL_TRY(oblt->p_glCompileShaderARB(s.f), SLP(s.f));
         MDFN_GL_TRY(oblt->p_glGetObjectParameterivARB(s.f, GL_OBJECT_COMPILE_STATUS_ARB, &opi));
         if(GL_FALSE == opi)
	 {
	  SLP(s.f);
	 }

         MDFN_GL_TRY(s.p = oblt->p_glCreateProgramObjectARB(), SLP(s.p));
	 s.p_valid = true;

         MDFN_GL_TRY(oblt->p_glAttachObjectARB(s.p, s.v));
         MDFN_GL_TRY(oblt->p_glAttachObjectARB(s.p, s.f));

         MDFN_GL_TRY(oblt->p_glLinkProgramARB(s.p), SLP(s.p));

         MDFN_GL_TRY(oblt->p_glDisable(GL_FRAGMENT_PROGRAM_ARB));
}

void OpenGL_Blitter_Shader::Cleanup(void)
{
        oblt->p_glUseProgramObjectARB(0);

	for(unsigned i = 0; i < CSP_COUNT; i++)
	{
	 if(CSP[i].p_valid)
	 {
	  if(CSP[i].f_valid)
	   oblt->p_glDetachObjectARB(CSP[i].p, CSP[i].f);
	  if(CSP[i].v_valid)
  	   oblt->p_glDetachObjectARB(CSP[i].p, CSP[i].v);
	 }
	 if(CSP[i].f_valid)
	  oblt->p_glDeleteObjectARB(CSP[i].f);
	 if(CSP[i].v_valid)
	  oblt->p_glDeleteObjectARB(CSP[i].v);
	 if(CSP[i].p_valid)
	  oblt->p_glDeleteObjectARB(CSP[i].p);

	 CSP[i].f_valid = CSP[i].v_valid = CSP[i].p_valid = false;
	}

        oblt->p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

OpenGL_Blitter_Shader::OpenGL_Blitter_Shader(OpenGL_Blitter *in_oblt, ShaderType shader_type) : oblt(in_oblt)
{
	OurType = shader_type;

	memset(&CSP, 0, sizeof(CSP));

	try
	{
	 switch(OurType)
	 {
	  case SHADER_SCALE2X:
		CompileShader(CSP[0], vertexProg, fragScale2X);
		break;

	  case SHADER_SABR:
		CompileShader(CSP[0], vertSABR, fragSABR);
		break;

	  default:
		for(unsigned i = 0; i < 8; i++)
		{
		 CompileShader(CSP[i], vertexProg, MakeProgIpolate(i).c_str());
		}
		break;
	 }
	}
	catch(...)
	{
	 Cleanup();
	 throw;
	}
}

void OpenGL_Blitter_Shader::ShaderBegin(const MDFN_Rect *rect, const MDFN_Rect *dest_rect, int tw, int th, int orig_tw, int orig_th, unsigned rotated)
{
        oblt->p_glEnable(GL_FRAGMENT_PROGRAM_ARB);

	//printf("%d:%d, %d:%d\n", tw, th, orig_tw, orig_th);
	//printf("%f\n", (double)dest_rect->w / rect->w);
        //printf("%f\n", (double)dest_rect->h / rect->h);
	if(OurType == SHADER_SCALE2X)
	{
 	 oblt->p_glUseProgramObjectARB(CSP[0].p);

         oblt->p_glUniform1iARB(oblt->p_glGetUniformLocationARB(CSP[0].p, "Tex0"), 0);
         oblt->p_glUniform2fARB(oblt->p_glGetUniformLocationARB(CSP[0].p, "TexSize"), tw, th);
         oblt->p_glUniform2fARB(oblt->p_glGetUniformLocationARB(CSP[0].p, "TexSizeInverse"), (float)1 / tw, (float) 1 / th);
	}
	else if(OurType == SHADER_SABR)
	{
 	 oblt->p_glUseProgramObjectARB(CSP[0].p);

         oblt->p_glUniform1iARB(oblt->p_glGetUniformLocationARB(CSP[0].p, "rubyTexture"), 0);
         oblt->p_glUniform2fARB(oblt->p_glGetUniformLocationARB(CSP[0].p, "rubyTextureSize"), tw, th);
	}
	else
	{
	 unsigned csi;
	 float sharpness = 2.0;
	 float xsh;
	 float ysh;
	 unsigned ip_x;
	 unsigned ip_y;
	 double drw_div_rw;
	 double drh_div_rh;

	 if(rotated == MDFN_ROTATE90 || rotated == MDFN_ROTATE270)
	 {
          drw_div_rw = (double)dest_rect->h / rect->w;
          drh_div_rh = (double)dest_rect->w / rect->h;
	 }
	 else
	 {
          drw_div_rw = (double)dest_rect->w / rect->w;
          drh_div_rh = (double)dest_rect->h / rect->h;
	 }

	 ip_x = 1;
	 ip_y = 1;

	 if(OurType == SHADER_AUTOIP)
	 {
	  xsh = 1;
	  ysh = 1;
	 }
	 else
	 {
	  xsh = drw_div_rw / sharpness;
	  ysh = drh_div_rh / sharpness;
	 }

	 if(OurType == SHADER_IPXNOTY || OurType == SHADER_IPXNOTYSHARPER)
	 {
	  if(OurType == SHADER_IPXNOTY)
	  {
	   xsh = 0;
	  }
	  ysh = 0;
	  ip_x = 1;
	  ip_y = 0;
	 }
	 else if(OurType == SHADER_IPYNOTX || OurType == SHADER_IPYNOTXSHARPER)
	 {
	  if(OurType == SHADER_IPYNOTX)
	  {
	   ysh = 0;
	  }
	  xsh = 0;
 	  ip_x = 0;
	  ip_y = 1;
	 }
	 else if(OurType == SHADER_IPSHARPER)
	 {
	  ip_x = true;
	  ip_y = true;
	 }
	 else
	 {
	  // Scaling X by an integer?
	  if(floor(drw_div_rw) == drw_div_rw)
	  {
	   xsh = 0;
	   ip_x = 0;
	  }

	  // Scaling Y by an integer?
	  if(floor(drh_div_rh) == drh_div_rh)
	  {
	   ysh = 0;
	   ip_y = 0;
	  }
	 }

	 if(xsh < 1)
	  xsh = 1;

	 if(ysh < 1)
	  ysh = 1;

	 csi = (ip_y << 1) | ip_x;
	 if(xsh > 1 && ip_x)
	  csi |= 4;
	 if(ysh > 1 && ip_y)
	  csi |= 4;

 	 oblt->p_glUseProgramObjectARB(CSP[csi].p);

//	printf("%d:%d, %d\n", ip_x, ip_y, csi);

         oblt->p_glUniform1iARB(oblt->p_glGetUniformLocationARB(CSP[csi].p, "Tex0"), 0);
         oblt->p_glUniform2fARB(oblt->p_glGetUniformLocationARB(CSP[csi].p, "TexSize"), tw, th);
         oblt->p_glUniform2fARB(oblt->p_glGetUniformLocationARB(CSP[csi].p, "TexSizeInverse"), (float)1 / tw, (float) 1 / th);

         oblt->p_glUniform1fARB(oblt->p_glGetUniformLocationARB(CSP[csi].p, "XSharp"), xsh);
         oblt->p_glUniform1fARB(oblt->p_glGetUniformLocationARB(CSP[csi].p, "YSharp"), ysh);
	}
}

void OpenGL_Blitter_Shader::ShaderEnd(void)
{
	oblt->p_glUseProgramObjectARB(0);
	oblt->p_glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

OpenGL_Blitter_Shader::~OpenGL_Blitter_Shader()
{
	Cleanup();
}

