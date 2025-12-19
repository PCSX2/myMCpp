/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gles1.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif



int GLAD_GL_VERSION_ES_CM_1_0 = 0;



PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLALPHAFUNCPROC glad_glAlphaFunc = NULL;
PFNGLALPHAFUNCXPROC glad_glAlphaFuncx = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARCOLORXPROC glad_glClearColorx = NULL;
PFNGLCLEARDEPTHFPROC glad_glClearDepthf = NULL;
PFNGLCLEARDEPTHXPROC glad_glClearDepthx = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCLIENTACTIVETEXTUREPROC glad_glClientActiveTexture = NULL;
PFNGLCLIPPLANEFPROC glad_glClipPlanef = NULL;
PFNGLCLIPPLANEXPROC glad_glClipPlanex = NULL;
PFNGLCOLOR4FPROC glad_glColor4f = NULL;
PFNGLCOLOR4UBPROC glad_glColor4ub = NULL;
PFNGLCOLOR4XPROC glad_glColor4x = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOLORPOINTERPROC glad_glColorPointer = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEFPROC glad_glDepthRangef = NULL;
PFNGLDEPTHRANGEXPROC glad_glDepthRangex = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLECLIENTSTATEPROC glad_glDisableClientState = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLECLIENTSTATEPROC glad_glEnableClientState = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFOGFPROC glad_glFogf = NULL;
PFNGLFOGFVPROC glad_glFogfv = NULL;
PFNGLFOGXPROC glad_glFogx = NULL;
PFNGLFOGXVPROC glad_glFogxv = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLFRUSTUMFPROC glad_glFrustumf = NULL;
PFNGLFRUSTUMXPROC glad_glFrustumx = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETCLIPPLANEFPROC glad_glGetClipPlanef = NULL;
PFNGLGETCLIPPLANEXPROC glad_glGetClipPlanex = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFIXEDVPROC glad_glGetFixedv = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETLIGHTFVPROC glad_glGetLightfv = NULL;
PFNGLGETLIGHTXVPROC glad_glGetLightxv = NULL;
PFNGLGETMATERIALFVPROC glad_glGetMaterialfv = NULL;
PFNGLGETMATERIALXVPROC glad_glGetMaterialxv = NULL;
PFNGLGETPOINTERVPROC glad_glGetPointerv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETTEXENVFVPROC glad_glGetTexEnvfv = NULL;
PFNGLGETTEXENVIVPROC glad_glGetTexEnviv = NULL;
PFNGLGETTEXENVXVPROC glad_glGetTexEnvxv = NULL;
PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv = NULL;
PFNGLGETTEXPARAMETERXVPROC glad_glGetTexParameterxv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLLIGHTMODELFPROC glad_glLightModelf = NULL;
PFNGLLIGHTMODELFVPROC glad_glLightModelfv = NULL;
PFNGLLIGHTMODELXPROC glad_glLightModelx = NULL;
PFNGLLIGHTMODELXVPROC glad_glLightModelxv = NULL;
PFNGLLIGHTFPROC glad_glLightf = NULL;
PFNGLLIGHTFVPROC glad_glLightfv = NULL;
PFNGLLIGHTXPROC glad_glLightx = NULL;
PFNGLLIGHTXVPROC glad_glLightxv = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINEWIDTHXPROC glad_glLineWidthx = NULL;
PFNGLLOADIDENTITYPROC glad_glLoadIdentity = NULL;
PFNGLLOADMATRIXFPROC glad_glLoadMatrixf = NULL;
PFNGLLOADMATRIXXPROC glad_glLoadMatrixx = NULL;
PFNGLLOGICOPPROC glad_glLogicOp = NULL;
PFNGLMATERIALFPROC glad_glMaterialf = NULL;
PFNGLMATERIALFVPROC glad_glMaterialfv = NULL;
PFNGLMATERIALXPROC glad_glMaterialx = NULL;
PFNGLMATERIALXVPROC glad_glMaterialxv = NULL;
PFNGLMATRIXMODEPROC glad_glMatrixMode = NULL;
PFNGLMULTMATRIXFPROC glad_glMultMatrixf = NULL;
PFNGLMULTMATRIXXPROC glad_glMultMatrixx = NULL;
PFNGLMULTITEXCOORD4FPROC glad_glMultiTexCoord4f = NULL;
PFNGLMULTITEXCOORD4XPROC glad_glMultiTexCoord4x = NULL;
PFNGLNORMAL3FPROC glad_glNormal3f = NULL;
PFNGLNORMAL3XPROC glad_glNormal3x = NULL;
PFNGLNORMALPOINTERPROC glad_glNormalPointer = NULL;
PFNGLORTHOFPROC glad_glOrthof = NULL;
PFNGLORTHOXPROC glad_glOrthox = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glad_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERXPROC glad_glPointParameterx = NULL;
PFNGLPOINTPARAMETERXVPROC glad_glPointParameterxv = NULL;
PFNGLPOINTSIZEPROC glad_glPointSize = NULL;
PFNGLPOINTSIZEXPROC glad_glPointSizex = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLPOLYGONOFFSETXPROC glad_glPolygonOffsetx = NULL;
PFNGLPOPMATRIXPROC glad_glPopMatrix = NULL;
PFNGLPUSHMATRIXPROC glad_glPushMatrix = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLROTATEFPROC glad_glRotatef = NULL;
PFNGLROTATEXPROC glad_glRotatex = NULL;
PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage = NULL;
PFNGLSAMPLECOVERAGEXPROC glad_glSampleCoveragex = NULL;
PFNGLSCALEFPROC glad_glScalef = NULL;
PFNGLSCALEXPROC glad_glScalex = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSHADEMODELPROC glad_glShadeModel = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLTEXCOORDPOINTERPROC glad_glTexCoordPointer = NULL;
PFNGLTEXENVFPROC glad_glTexEnvf = NULL;
PFNGLTEXENVFVPROC glad_glTexEnvfv = NULL;
PFNGLTEXENVIPROC glad_glTexEnvi = NULL;
PFNGLTEXENVIVPROC glad_glTexEnviv = NULL;
PFNGLTEXENVXPROC glad_glTexEnvx = NULL;
PFNGLTEXENVXVPROC glad_glTexEnvxv = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXPARAMETERXPROC glad_glTexParameterx = NULL;
PFNGLTEXPARAMETERXVPROC glad_glTexParameterxv = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLTRANSLATEFPROC glad_glTranslatef = NULL;
PFNGLTRANSLATEXPROC glad_glTranslatex = NULL;
PFNGLVERTEXPOINTERPROC glad_glVertexPointer = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;


static void glad_gl_load_GL_VERSION_ES_CM_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_ES_CM_1_0) return;
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
    glad_glAlphaFunc = (PFNGLALPHAFUNCPROC) load(userptr, "glAlphaFunc");
    glad_glAlphaFuncx = (PFNGLALPHAFUNCXPROC) load(userptr, "glAlphaFuncx");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
    glad_glBindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    glad_glBlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    glad_glBufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
    glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
    glad_glClear = (PFNGLCLEARPROC) load(userptr, "glClear");
    glad_glClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    glad_glClearColorx = (PFNGLCLEARCOLORXPROC) load(userptr, "glClearColorx");
    glad_glClearDepthf = (PFNGLCLEARDEPTHFPROC) load(userptr, "glClearDepthf");
    glad_glClearDepthx = (PFNGLCLEARDEPTHXPROC) load(userptr, "glClearDepthx");
    glad_glClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    glad_glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) load(userptr, "glClientActiveTexture");
    glad_glClipPlanef = (PFNGLCLIPPLANEFPROC) load(userptr, "glClipPlanef");
    glad_glClipPlanex = (PFNGLCLIPPLANEXPROC) load(userptr, "glClipPlanex");
    glad_glColor4f = (PFNGLCOLOR4FPROC) load(userptr, "glColor4f");
    glad_glColor4ub = (PFNGLCOLOR4UBPROC) load(userptr, "glColor4ub");
    glad_glColor4x = (PFNGLCOLOR4XPROC) load(userptr, "glColor4x");
    glad_glColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    glad_glColorPointer = (PFNGLCOLORPOINTERPROC) load(userptr, "glColorPointer");
    glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    glad_glCullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
    glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    glad_glDepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    glad_glDepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    glad_glDepthRangef = (PFNGLDEPTHRANGEFPROC) load(userptr, "glDepthRangef");
    glad_glDepthRangex = (PFNGLDEPTHRANGEXPROC) load(userptr, "glDepthRangex");
    glad_glDisable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    glad_glDisableClientState = (PFNGLDISABLECLIENTSTATEPROC) load(userptr, "glDisableClientState");
    glad_glDrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    glad_glDrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    glad_glEnable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    glad_glEnableClientState = (PFNGLENABLECLIENTSTATEPROC) load(userptr, "glEnableClientState");
    glad_glFinish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    glad_glFlush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    glad_glFogf = (PFNGLFOGFPROC) load(userptr, "glFogf");
    glad_glFogfv = (PFNGLFOGFVPROC) load(userptr, "glFogfv");
    glad_glFogx = (PFNGLFOGXPROC) load(userptr, "glFogx");
    glad_glFogxv = (PFNGLFOGXVPROC) load(userptr, "glFogxv");
    glad_glFrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    glad_glFrustumf = (PFNGLFRUSTUMFPROC) load(userptr, "glFrustumf");
    glad_glFrustumx = (PFNGLFRUSTUMXPROC) load(userptr, "glFrustumx");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
    glad_glGenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
    glad_glGetClipPlanef = (PFNGLGETCLIPPLANEFPROC) load(userptr, "glGetClipPlanef");
    glad_glGetClipPlanex = (PFNGLGETCLIPPLANEXPROC) load(userptr, "glGetClipPlanex");
    glad_glGetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    glad_glGetFixedv = (PFNGLGETFIXEDVPROC) load(userptr, "glGetFixedv");
    glad_glGetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    glad_glGetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    glad_glGetLightfv = (PFNGLGETLIGHTFVPROC) load(userptr, "glGetLightfv");
    glad_glGetLightxv = (PFNGLGETLIGHTXVPROC) load(userptr, "glGetLightxv");
    glad_glGetMaterialfv = (PFNGLGETMATERIALFVPROC) load(userptr, "glGetMaterialfv");
    glad_glGetMaterialxv = (PFNGLGETMATERIALXVPROC) load(userptr, "glGetMaterialxv");
    glad_glGetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    glad_glGetTexEnvfv = (PFNGLGETTEXENVFVPROC) load(userptr, "glGetTexEnvfv");
    glad_glGetTexEnviv = (PFNGLGETTEXENVIVPROC) load(userptr, "glGetTexEnviv");
    glad_glGetTexEnvxv = (PFNGLGETTEXENVXVPROC) load(userptr, "glGetTexEnvxv");
    glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    glad_glGetTexParameterxv = (PFNGLGETTEXPARAMETERXVPROC) load(userptr, "glGetTexParameterxv");
    glad_glHint = (PFNGLHINTPROC) load(userptr, "glHint");
    glad_glIsBuffer = (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer");
    glad_glIsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    glad_glIsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    glad_glLightModelf = (PFNGLLIGHTMODELFPROC) load(userptr, "glLightModelf");
    glad_glLightModelfv = (PFNGLLIGHTMODELFVPROC) load(userptr, "glLightModelfv");
    glad_glLightModelx = (PFNGLLIGHTMODELXPROC) load(userptr, "glLightModelx");
    glad_glLightModelxv = (PFNGLLIGHTMODELXVPROC) load(userptr, "glLightModelxv");
    glad_glLightf = (PFNGLLIGHTFPROC) load(userptr, "glLightf");
    glad_glLightfv = (PFNGLLIGHTFVPROC) load(userptr, "glLightfv");
    glad_glLightx = (PFNGLLIGHTXPROC) load(userptr, "glLightx");
    glad_glLightxv = (PFNGLLIGHTXVPROC) load(userptr, "glLightxv");
    glad_glLineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    glad_glLineWidthx = (PFNGLLINEWIDTHXPROC) load(userptr, "glLineWidthx");
    glad_glLoadIdentity = (PFNGLLOADIDENTITYPROC) load(userptr, "glLoadIdentity");
    glad_glLoadMatrixf = (PFNGLLOADMATRIXFPROC) load(userptr, "glLoadMatrixf");
    glad_glLoadMatrixx = (PFNGLLOADMATRIXXPROC) load(userptr, "glLoadMatrixx");
    glad_glLogicOp = (PFNGLLOGICOPPROC) load(userptr, "glLogicOp");
    glad_glMaterialf = (PFNGLMATERIALFPROC) load(userptr, "glMaterialf");
    glad_glMaterialfv = (PFNGLMATERIALFVPROC) load(userptr, "glMaterialfv");
    glad_glMaterialx = (PFNGLMATERIALXPROC) load(userptr, "glMaterialx");
    glad_glMaterialxv = (PFNGLMATERIALXVPROC) load(userptr, "glMaterialxv");
    glad_glMatrixMode = (PFNGLMATRIXMODEPROC) load(userptr, "glMatrixMode");
    glad_glMultMatrixf = (PFNGLMULTMATRIXFPROC) load(userptr, "glMultMatrixf");
    glad_glMultMatrixx = (PFNGLMULTMATRIXXPROC) load(userptr, "glMultMatrixx");
    glad_glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC) load(userptr, "glMultiTexCoord4f");
    glad_glMultiTexCoord4x = (PFNGLMULTITEXCOORD4XPROC) load(userptr, "glMultiTexCoord4x");
    glad_glNormal3f = (PFNGLNORMAL3FPROC) load(userptr, "glNormal3f");
    glad_glNormal3x = (PFNGLNORMAL3XPROC) load(userptr, "glNormal3x");
    glad_glNormalPointer = (PFNGLNORMALPOINTERPROC) load(userptr, "glNormalPointer");
    glad_glOrthof = (PFNGLORTHOFPROC) load(userptr, "glOrthof");
    glad_glOrthox = (PFNGLORTHOXPROC) load(userptr, "glOrthox");
    glad_glPixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    glad_glPointParameterf = (PFNGLPOINTPARAMETERFPROC) load(userptr, "glPointParameterf");
    glad_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC) load(userptr, "glPointParameterfv");
    glad_glPointParameterx = (PFNGLPOINTPARAMETERXPROC) load(userptr, "glPointParameterx");
    glad_glPointParameterxv = (PFNGLPOINTPARAMETERXVPROC) load(userptr, "glPointParameterxv");
    glad_glPointSize = (PFNGLPOINTSIZEPROC) load(userptr, "glPointSize");
    glad_glPointSizex = (PFNGLPOINTSIZEXPROC) load(userptr, "glPointSizex");
    glad_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    glad_glPolygonOffsetx = (PFNGLPOLYGONOFFSETXPROC) load(userptr, "glPolygonOffsetx");
    glad_glPopMatrix = (PFNGLPOPMATRIXPROC) load(userptr, "glPopMatrix");
    glad_glPushMatrix = (PFNGLPUSHMATRIXPROC) load(userptr, "glPushMatrix");
    glad_glReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    glad_glRotatef = (PFNGLROTATEFPROC) load(userptr, "glRotatef");
    glad_glRotatex = (PFNGLROTATEXPROC) load(userptr, "glRotatex");
    glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
    glad_glSampleCoveragex = (PFNGLSAMPLECOVERAGEXPROC) load(userptr, "glSampleCoveragex");
    glad_glScalef = (PFNGLSCALEFPROC) load(userptr, "glScalef");
    glad_glScalex = (PFNGLSCALEXPROC) load(userptr, "glScalex");
    glad_glScissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    glad_glShadeModel = (PFNGLSHADEMODELPROC) load(userptr, "glShadeModel");
    glad_glStencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    glad_glStencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    glad_glStencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    glad_glTexCoordPointer = (PFNGLTEXCOORDPOINTERPROC) load(userptr, "glTexCoordPointer");
    glad_glTexEnvf = (PFNGLTEXENVFPROC) load(userptr, "glTexEnvf");
    glad_glTexEnvfv = (PFNGLTEXENVFVPROC) load(userptr, "glTexEnvfv");
    glad_glTexEnvi = (PFNGLTEXENVIPROC) load(userptr, "glTexEnvi");
    glad_glTexEnviv = (PFNGLTEXENVIVPROC) load(userptr, "glTexEnviv");
    glad_glTexEnvx = (PFNGLTEXENVXPROC) load(userptr, "glTexEnvx");
    glad_glTexEnvxv = (PFNGLTEXENVXVPROC) load(userptr, "glTexEnvxv");
    glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    glad_glTexParameterx = (PFNGLTEXPARAMETERXPROC) load(userptr, "glTexParameterx");
    glad_glTexParameterxv = (PFNGLTEXPARAMETERXVPROC) load(userptr, "glTexParameterxv");
    glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
    glad_glTranslatef = (PFNGLTRANSLATEFPROC) load(userptr, "glTranslatef");
    glad_glTranslatex = (PFNGLTRANSLATEXPROC) load(userptr, "glTranslatex");
    glad_glVertexPointer = (PFNGLVERTEXPOINTERPROC) load(userptr, "glVertexPointer");
    glad_glViewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}



static void glad_gl_free_extensions(char **exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for(index = 0; exts_i[index]; index++) {
            free((void *) (exts_i[index]));
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}
static int glad_gl_get_extensions( const char **out_exts, char ***out_exts_i) {
#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
    if (glad_glGetStringi != NULL && glad_glGetIntegerv != NULL) {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char **exts_i = NULL;
        glad_glGetIntegerv(GL_NUM_EXTENSIONS, (int*) &num_exts_i);
        exts_i = (char **) malloc((num_exts_i + 1) * (sizeof *exts_i));
        if (exts_i == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts_i; index++) {
            const char *gl_str_tmp = (const char*) glad_glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char *local_str = (char*) malloc(len * sizeof(char));
            if(local_str == NULL) {
                exts_i[index] = NULL;
                glad_gl_free_extensions(exts_i);
                return 0;
            }

            memcpy(local_str, gl_str_tmp, len * sizeof(char));
            exts_i[index] = local_str;
        }
        exts_i[index] = NULL;

        *out_exts_i = exts_i;

        return 1;
    }
#else
    GLAD_UNUSED(out_exts_i);
#endif
    if (glad_glGetString == NULL) {
        return 0;
    }
    *out_exts = (const char *)glad_glGetString(GL_EXTENSIONS);
    return 1;
}
static int glad_gl_has_extension(const char *exts, char **exts_i, const char *ext) {
    if(exts_i) {
        unsigned int index;
        for(index = 0; exts_i[index]; index++) {
            const char *e = exts_i[index];
            if(strcmp(e, ext) == 0) {
                return 1;
            }
        }
    } else {
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }
        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    }
    return 0;
}

static GLADapiproc glad_gl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_gl_find_extensions_gles1(void) {
    const char *exts = NULL;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(&exts, &exts_i)) return 0;

    GLAD_UNUSED(&glad_gl_has_extension);

    glad_gl_free_extensions(exts_i);

    return 1;
}

static int glad_gl_find_core_gles1(void) {
    int i;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        "OpenGL SC ",
        NULL
    };
    int major = 0;
    int minor = 0;
    version = (const char*) glad_glGetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_GL_VERSION_ES_CM_1_0 = (major == 1 && minor >= 0) || major > 1;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLES1UserPtr( GLADuserptrloadfunc load, void *userptr) {
    int version;

    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if(glad_glGetString == NULL) return 0;
    version = glad_gl_find_core_gles1();

    glad_gl_load_GL_VERSION_ES_CM_1_0(load, userptr);

    if (!glad_gl_find_extensions_gles1()) return 0;



    return version;
}


int gladLoadGLES1( GLADloadfunc load) {
    return gladLoadGLES1UserPtr( glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}



 


#ifdef __cplusplus
}
#endif
