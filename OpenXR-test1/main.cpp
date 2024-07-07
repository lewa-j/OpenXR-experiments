
#pragma warning(disable : 26812) //The enum type 'XrResult' is unscoped.Prefer 'enum class' over 'enum' (Enum.3)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <vector>
//#define XR_NO_PROTOTYPES 1
#include "openxr/openxr.h"
//#define XR_USE_GRAPHICS_API_VULKAN 1
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#include <Windows.h>
#include "openxr/openxr_platform.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#define GLFW_EXPOSE_NATIVE_WGL 1
#include <GLFW/glfw3native.h>

#include "GLSLShader.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>

#include <openxr/openxr_reflection.h>

#define XR_ENUM_CASE_STR(name, val) case name: return #name;
#define XR_ENUM_STR(enumType)                         \
const char* XrEnumStr(enumType e) {     \
    switch (e) {                                  \
        XR_LIST_ENUM_##enumType(XR_ENUM_CASE_STR) \
        default: return "Unknown";                \
    }                                             \
}                                                 \

XR_ENUM_STR(XrStructureType)
XR_ENUM_STR(XrSessionState)
XR_ENUM_STR(XrViewConfigurationType)
XR_ENUM_STR(XrEnvironmentBlendMode)
XR_ENUM_STR(XrReferenceSpaceType)

const char* glfmt_to_str(int f);

/*PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr;
PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
PFN_xrCreateInstance xrCreateInstance;
PFN_xrDestroyInstance xrDestroyInstance;
PFN_xrPollEvent xrPollEvent;
*/

PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXTd;
PFN_xrDestroyDebugUtilsMessengerEXT xrDestroyDebugUtilsMessengerEXTd;
PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHRd;

#define Log printf



XrBool32 DebugMessengerCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes, const XrDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
	Log("%s %s %s\n",callbackData->messageId,callbackData->functionName,callbackData->message);
	return XR_FALSE;
}

const char* vertex_shader = R"(
#version 100
precision highp float;
attribute vec4 a_position;
uniform mat4 u_mvpMtx;

void main()
{
	gl_Position = u_mvpMtx * a_position;
	gl_PointSize = 4.0;
}
)";

const char* pixel_shader = R"(
#version 100
precision highp float;
uniform vec4 u_color;

void main()
{
	gl_FragColor = vec4(1.0);
}
)";

float cubeVerts[]{
	-1,-1,-1,
	1,-1,-1,
	-1,1,-1,
	1,1,-1,
	-1,-1,1,
	1,-1,1,
	-1,1,1,
	1,1,1
};

uint16_t cubeInds[]{
	0,1,
	0,2,
	2,3,
	1,3,

	4,5,
	4,6,
	6,7,
	5,7,

	0,4,
	1,5,
	2,6,
	3,7
};

glm::mat4 FrustumXR(XrFovf fov, float nearf, float farf)
{
	return glm::frustum(nearf*tan(fov.angleLeft), nearf * tan(fov.angleRight), nearf * tan(fov.angleDown), nearf * tan(fov.angleUp), nearf, farf);
}

bool CheckXrResult(XrResult r, const char *func)
{
	//if (XR_FAILED(r)) 
	if(r != XR_SUCCESS)
	{
		char buffer[XR_MAX_RESULT_STRING_SIZE] = "";
		xrResultToString(XR_NULL_HANDLE, r, buffer);
		Log("%s failed %s\n", func, buffer);
		return true;
	}
	return false;
}

int main(int carc, const char** argv)
{
	printf("Starting\n");

	HDC dc{};
	HGLRC glrc{};

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(800, 480, "OpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Error: flad load failed\n");
		return EXIT_FAILURE;
	}

	dc = GetDC(glfwGetWin32Window(window));
	//dc = GetDC(NULL);
	glrc = glfwGetWGLContext(window);
	printf("gwfw dc %p glrc %p\n", dc, glrc);


	GLSLShader simpleShader;
	//if (simpleShader.Load("simple", "simple"))
	if (simpleShader.Create(vertex_shader, pixel_shader))
		return EXIT_FAILURE;

	//init
	XrResult r = XR_SUCCESS;
	/*	r = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrEnumerateApiLayerProperties", (PFN_xrVoidFunction*)&xrEnumerateApiLayerProperties);
		r = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties);
		r = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrCreateInstance", (PFN_xrVoidFunction*)&xrCreateInstance);

		Log("xrEnumerateApiLayerProperties %p\n", xrEnumerateApiLayerProperties);
		Log("xrEnumerateInstanceExtensionProperties %p\n", xrEnumerateInstanceExtensionProperties);
		Log("xrCreateInstance %p\n", xrCreateInstance);
	*/

	uint32_t layerCount = 0;
	r = xrEnumerateApiLayerProperties(0, &layerCount, nullptr);
	Log("%d api layers %d\n", r, layerCount);
	if (layerCount)
	{
		std::vector<XrApiLayerProperties> layers(layerCount, { XR_TYPE_API_LAYER_PROPERTIES });
		xrEnumerateApiLayerProperties(layerCount, &layerCount, layers.data());
		for (const auto& l : layers)
			Log("  %s: %s\n", l.layerName, l.description);
	}
	uint32_t extCount = 0;
	r = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
	Log("%d inst extensions %d\n", r, extCount);
	std::vector<XrExtensionProperties> exts(extCount, { XR_TYPE_EXTENSION_PROPERTIES });
	if (extCount)
		xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, exts.data());

	std::vector <const char*> enabledExts;

	bool have_EXT_debug_utils = false;
	bool have_KHR_opengl_enable = false;
	for (int i = 0; i < exts.size(); i++)
	{
		Log(" %d: %s v%d\n", i, exts[i].extensionName, exts[i].extensionVersion);

		if (!strcmp(exts[i].extensionName, "XR_EXT_debug_utils"))
			have_EXT_debug_utils = true;

		if (!strcmp(exts[i].extensionName, "XR_KHR_opengl_enable"))
			have_KHR_opengl_enable = true;
	}

	if (have_EXT_debug_utils)
		enabledExts.push_back("XR_EXT_debug_utils");
	if (have_KHR_opengl_enable)
		enabledExts.push_back("XR_KHR_opengl_enable");


	XrInstance instance = XR_NULL_HANDLE;
	XrInstanceCreateInfo instInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	strncpy(instInfo.applicationInfo.applicationName, "xr-test", XR_MAX_APPLICATION_NAME_SIZE);
	instInfo.applicationInfo.applicationVersion = 1;
	strncpy(instInfo.applicationInfo.engineName, "", XR_MAX_ENGINE_NAME_SIZE);
	instInfo.applicationInfo.engineVersion = 1;
	instInfo.enabledExtensionCount = enabledExts.size();
	instInfo.enabledExtensionNames = enabledExts.data();

	instInfo.applicationInfo.apiVersion = XR_API_VERSION_1_1;
	r = xrCreateInstance(&instInfo, &instance);
	Log("%d create instance 1.1 %p\n", r, instance);
	if (r == XR_ERROR_API_VERSION_UNSUPPORTED)
	{
		instInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
		r = xrCreateInstance(&instInfo, &instance);
		Log("%d create instance 1.0 %p\n", r, instance);
	}

	if (XR_FAILED(r))
	{
		CheckXrResult(r, "xrCreateInstance");
		return -1;
	}

	//r = xrGetInstanceProcAddr(instance, "xrDestroyInstance", (PFN_xrVoidFunction*)&xrDestroyInstance);
	//r = xrGetInstanceProcAddr(instance, "xrPollEvent", (PFN_xrVoidFunction*)&xrPollEvent);
	if (have_EXT_debug_utils)
	{
		r = xrGetInstanceProcAddr(instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)&xrCreateDebugUtilsMessengerEXTd);
		r = xrGetInstanceProcAddr(instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)&xrDestroyDebugUtilsMessengerEXTd);
	}
	if (have_KHR_opengl_enable)
		r = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetOpenGLGraphicsRequirementsKHRd);

	XrDebugUtilsMessengerEXT debugMessenger = XR_NULL_HANDLE;

	if (have_EXT_debug_utils)
	{
		XrDebugUtilsMessengerCreateInfoEXT messengerInfo{ XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, nullptr,
			XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT,
			DebugMessengerCallback, nullptr };
		r = xrCreateDebugUtilsMessengerEXTd(instance, &messengerInfo, &debugMessenger);
		Log("%d DebugUtilsMessenger %p\n", r, debugMessenger);
	}

	XrInstanceProperties instProps{ XR_TYPE_INSTANCE_PROPERTIES };
	r = xrGetInstanceProperties(instance, &instProps);
	Log("%d inst props: runtime: %s: %d.%d.%d\n", r, instProps.runtimeName, XR_VERSION_MAJOR(instProps.runtimeVersion), XR_VERSION_MINOR(instProps.runtimeVersion), XR_VERSION_PATCH(instProps.runtimeVersion));

	XrSystemId systemId = XR_NULL_SYSTEM_ID;
	XrFormFactor formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemGetInfo sysGetInfo{ XR_TYPE_SYSTEM_GET_INFO, nullptr, formFactor };
	r = xrGetSystem(instance, &sysGetInfo, &systemId);
	if (XR_FAILED(r))
	{
		CheckXrResult(r, "xrGetSystem");
		xrDestroyInstance(instance);
		return -1;
	}
	Log("%d system %llX\n", r, systemId);

	XrSystemProperties sysProps{ XR_TYPE_SYSTEM_PROPERTIES };
	r = xrGetSystemProperties(instance, systemId, &sysProps);
	Log("%d system props: vendor %X, %s", r, sysProps.vendorId, sysProps.systemName);
	Log(" graphics: max res %dx%d layers %d",
		sysProps.graphicsProperties.maxSwapchainImageWidth,
		sysProps.graphicsProperties.maxSwapchainImageHeight,
		sysProps.graphicsProperties.maxLayerCount);
	Log(" tracking: orientation %d, position %d\n", sysProps.trackingProperties.orientationTracking, sysProps.trackingProperties.positionTracking);

	XrViewConfigurationType viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	uint32_t viewConfigCount = 0;
	r = xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigCount, nullptr);
	Log("%d view configs %d\n", r, viewConfigCount);
	std::vector<XrViewConfigurationType> viewConfigs(viewConfigCount);
	if (viewConfigCount)
		r = xrEnumerateViewConfigurations(instance, systemId, (uint32_t)viewConfigs.size(), &viewConfigCount, viewConfigs.data());
	for (int i = 0; i < viewConfigCount; i++)
	{
		XrViewConfigurationProperties viewConfigProps{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
		r = xrGetViewConfigurationProperties(instance, systemId, viewConfigs[i], &viewConfigProps);
		Log(" %d: %X %s fovMutable %d\n", i, viewConfigs[i], XrEnumStr(viewConfigs[i]), viewConfigProps.fovMutable);
	}

	uint32_t cfgViewCount = 0;
	std::vector<XrViewConfigurationView> cfgViews;
	r = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, 0, &cfgViewCount, nullptr);
	Log("%d view cfg views %d\n", r, cfgViewCount);
	if (viewConfigCount)
	{
		cfgViews.resize(cfgViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr });
		r = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, (uint32_t)cfgViews.size(), &cfgViewCount, cfgViews.data());
		for (int i = 0; i < 2; i++)
		{
			Log(" %d: recomended res %dx%d samples %d, max res %dx%d samples %d\n", i,
				cfgViews[i].recommendedImageRectWidth,
				cfgViews[i].recommendedImageRectHeight,
				cfgViews[i].recommendedSwapchainSampleCount,
				cfgViews[i].maxImageRectWidth,
				cfgViews[i].maxImageRectHeight,
				cfgViews[i].maxSwapchainSampleCount);
		}
	}

	uint32_t envBlendModesCount = 0;
	r = xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, 0, &envBlendModesCount, nullptr);
	Log("%d Environment Blend Modes %d\n", r, envBlendModesCount);
	if (envBlendModesCount)
	{
		std::vector<XrEnvironmentBlendMode> blendModes(envBlendModesCount);
		r = xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, blendModes.size(), &envBlendModesCount, blendModes.data());
		for (int i = 0; i < envBlendModesCount; i++)
			Log(" %d: %X %s\n", i, blendModes[i], XrEnumStr(blendModes[i]));
	}

	//input

	XrPath handsPaths[2]{};
	r = xrStringToPath(instance, "/user/hand/left", &handsPaths[0]);
	r = xrStringToPath(instance, "/user/hand/right", &handsPaths[1]);

	XrActionSet actionSet;
	XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO,nullptr,"main","Main",0 };
	r = xrCreateActionSet(instance, &actionSetInfo, &actionSet);
	CheckXrResult(r, "xrCreateActionSet");

	XrAction handAction;
	XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO,nullptr,"hand_pose",XR_ACTION_TYPE_POSE_INPUT,2,handsPaths,"Hand pose" };
	r = xrCreateAction(actionSet, &actionInfo, &handAction);
	CheckXrResult(r, "xrCreateAction hand pose");

	XrAction clickAction;
	strcpy(actionInfo.actionName, "click");
	strcpy(actionInfo.localizedActionName, "Click");
	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	r = xrCreateAction(actionSet, &actionInfo, &clickAction);
	Log("%d action click %p\n", r, clickAction);

	XrActionSuggestedBinding bindings[4];
	bindings[0].action = handAction;
	bindings[1].action = handAction;
	xrStringToPath(instance, "/user/hand/left/input/grip", &bindings[0].binding);
	xrStringToPath(instance, "/user/hand/right/input/grip", &bindings[1].binding);
	bindings[2].action = clickAction;
	bindings[3].action = clickAction;
	xrStringToPath(instance, "/user/hand/left/input/trigger", &(bindings[2].binding));
	xrStringToPath(instance, "/user/hand/right/input/trigger", &(bindings[3].binding));

	XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggestedBindings.countSuggestedBindings = 4;
	suggestedBindings.suggestedBindings = bindings;

	xrStringToPath(instance, "/interaction_profiles/htc/vive_controller", &suggestedBindings.interactionProfile);
	r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
	Log("%d suggestInteractionProfileBindings htc/vive_controller\n", r);
	CheckXrResult(r, "xrSuggestInteractionProfileBindings vive");

	{
		xrStringToPath(instance, "/interaction_profiles/microsoft/motion_controller", &suggestedBindings.interactionProfile);
		r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		Log("%d suggestInteractionProfileBindings microsoft/motion_controller\n", r);

		// 1.1 standard

		xrStringToPath(instance, "/interaction_profiles/bytedance/pico_neo3_controller", &suggestedBindings.interactionProfile);
		r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		Log("%d suggestInteractionProfileBindings bytedance/pico_neo3_controller\n", r);

		xrStringToPath(instance, "/interaction_profiles/bytedance/pico4_controller", &suggestedBindings.interactionProfile);
		r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		Log("%d suggestInteractionProfileBindings bytedance/pico4_controller\n", r);

		//non standart
		xrStringToPath(instance, "/interaction_profiles/pico/neo3_controller", &suggestedBindings.interactionProfile);
		r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		Log("%d suggestInteractionProfileBindings pico/neo3_controller\n", r);

		//if (xr_11)
		{
			xrStringToPath(instance, "/interaction_profiles/meta/touch_controller_quest_2", &suggestedBindings.interactionProfile);
			r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
			Log("%d suggestInteractionProfileBindings meta/touch_controller_quest_2\n", r);
		}
		//else
		{
			xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &suggestedBindings.interactionProfile);
			r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
			Log("%d suggestInteractionProfileBindings oculus/touch_controller\n", r);

		}

		xrStringToPath(instance, "/interaction_profiles/valve/index_controller", &suggestedBindings.interactionProfile);
		r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		Log("%d suggestInteractionProfileBindings valve/index_controller\n", r);
	}

	xrStringToPath(instance, "/interaction_profiles/khr/simple_controller", &suggestedBindings.interactionProfile);
	xrStringToPath(instance, "/user/hand/left/input/select", &(bindings[2].binding));
	xrStringToPath(instance, "/user/hand/right/input/select", &(bindings[3].binding));
	r = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
	Log("%d suggestInteractionProfileBindings khr/simple_controller\n", r);
	CheckXrResult(r, "xrSuggestInteractionProfileBindings khr/simple_controller");

	//session

	XrGraphicsRequirementsOpenGLKHR glReqs{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	r = xrGetOpenGLGraphicsRequirementsKHRd(instance, systemId, &glReqs);
	Log("OpenGL %d.%d - %d.%d required\n", XR_VERSION_MAJOR(glReqs.minApiVersionSupported), XR_VERSION_MINOR(glReqs.minApiVersionSupported), XR_VERSION_MAJOR(glReqs.maxApiVersionSupported), XR_VERSION_MINOR(glReqs.maxApiVersionSupported));

	XrSession session = XR_NULL_HANDLE;
	XrGraphicsBindingOpenGLWin32KHR glWin32Binding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, nullptr, dc, glrc };
	XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO, &glWin32Binding, 0, systemId };
	r = xrCreateSession(instance, &sessionInfo, &session);
	if (XR_FAILED(r)) {
		CheckXrResult(r, "xrCreateSession");
		xrDestroyInstance(instance);
		return -1;
	}
	Log("%d session %p\n", r, session);

	// spaces
	uint32_t refSpacesCount = 0;
	r = xrEnumerateReferenceSpaces(session, 0, &refSpacesCount, nullptr);
	Log("%d reference space types %d\n", r, refSpacesCount);
	if (refSpacesCount)
	{
		std::vector<XrReferenceSpaceType> refSpaceTypes(refSpacesCount);
		r = xrEnumerateReferenceSpaces(session, refSpaceTypes.size(), &refSpacesCount, refSpaceTypes.data());
		for (int i = 0; i < refSpacesCount; i++)
		{
			XrExtent2Df bounds{};
			XrResult br = xrGetReferenceSpaceBoundsRect(session, refSpaceTypes[i], &bounds);
			Log(" %d: %X %s, %d bounds %fx%f\n", i, refSpaceTypes[i], XrEnumStr(refSpaceTypes[i]), br, bounds.width, bounds.height);
		}
	}

	XrSpace localSpace{};
	XrReferenceSpaceCreateInfo refSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	refSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	refSpaceCreateInfo.poseInReferenceSpace = { {0,0,0,1},{0,0,0} };
	r = xrCreateReferenceSpace(session, &refSpaceCreateInfo, &localSpace);
	if (XR_FAILED(r)) {
		char buffer[XR_MAX_RESULT_STRING_SIZE] = "";
		xrResultToString(XR_NULL_HANDLE, r, buffer);
		printf("xrCreateReferenceSpace failed %s\n", buffer);
	}
	Log("%d create local ref space %p\n", r, localSpace);

	// swapchain

	uint32_t formatsCount = 0;
	r = xrEnumerateSwapchainFormats(session, 0, &formatsCount, nullptr);
	Log("%d swapchain formats: %d\n", r, formatsCount);
	int64_t swapchainFormat = 0;
	if (formatsCount)
	{
		std::vector<int64_t>swapchainFormats(formatsCount);
		r = xrEnumerateSwapchainFormats(session, formatsCount, &formatsCount, swapchainFormats.data());
		for (int i = 0; i < formatsCount; i++)
		{
			Log(" %d: %llX %s\n", i, swapchainFormats[i], glfmt_to_str(swapchainFormats[i]));
			if (swapchainFormats[i] == GL_SRGB8_ALPHA8)
				swapchainFormat = swapchainFormats[i];
		}

		if (swapchainFormat == 0)
			swapchainFormat = swapchainFormats[0];
	}

	struct viewData
	{
		XrSwapchain swapchain;
		std::vector<XrSwapchainImageOpenGLKHR> images;
		XrRect2Di rect;
		uint32_t fbo;
		uint32_t rbDepth;
	};
	std::vector<viewData> viewsData{ cfgViews.size() };

	for (int i = 0; i < cfgViews.size(); i++)
	{
		XrSwapchainCreateInfo scInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
		scInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		scInfo.format = swapchainFormat;
		scInfo.sampleCount = cfgViews[i].recommendedSwapchainSampleCount;
		scInfo.width = cfgViews[i].recommendedImageRectWidth;
		scInfo.height = cfgViews[i].recommendedImageRectHeight;
		scInfo.faceCount = 1;
		scInfo.arraySize = 1;
		scInfo.mipCount = 1;
		r = xrCreateSwapchain(session, &scInfo, &viewsData[i].swapchain);
		if (XR_FAILED(r)) {
			char buffer[XR_MAX_RESULT_STRING_SIZE] = "";
			xrResultToString(XR_NULL_HANDLE, r, buffer);
			printf("xrCreateSwapchain %d failed %s\n", i, buffer);
			xrDestroySession(session);
			xrDestroyInstance(instance);
			return -1;
		}
		Log("%d swapchain[%d] %p\n", r, i, viewsData[i].swapchain);
		viewsData[i].rect = { {0,0}, {(int32_t)cfgViews[i].recommendedImageRectWidth, (int32_t)cfgViews[i].recommendedImageRectHeight} };

		uint32_t swapchainImgCount = 0;
		r = xrEnumerateSwapchainImages(viewsData[i].swapchain, 0, &swapchainImgCount, nullptr);
		Log("%d images count %d\n", r, swapchainImgCount);
		if (swapchainImgCount)
		{
			viewsData[i].images.resize(swapchainImgCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
			r = xrEnumerateSwapchainImages(viewsData[i].swapchain, swapchainImgCount, &swapchainImgCount, (XrSwapchainImageBaseHeader*)viewsData[i].images.data());

			for (int j = 0; j < swapchainImgCount; j++)
				Log(" %d: %d\n", j, viewsData[i].images[j].image);
		}
	}

	XrSpace gripSpaces[2]{};
	XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO,nullptr,handAction,handsPaths[0],{{0,0,0,1},{0,0,0}} };
	r = xrCreateActionSpace(session, &actionSpaceInfo, gripSpaces);
	CheckXrResult(r,"xrCreateActionSpace l");
	actionSpaceInfo.subactionPath = handsPaths[1];
	r = xrCreateActionSpace(session, &actionSpaceInfo, gripSpaces+1);
	CheckXrResult(r,"xrCreateActionSpace r");

	XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,nullptr,1,&actionSet};
	r = xrAttachSessionActionSets(session, &attachInfo);
	CheckXrResult(r, "xrAttachSessionActionSets");
	Log("%d attachSessionActionSets\n", r);

	XrInteractionProfileState profile{ XR_TYPE_INTERACTION_PROFILE_STATE };
	r = xrGetCurrentInteractionProfile(session, handsPaths[1], &profile);
	char profileStr[XR_MAX_PATH_LENGTH]{ 0 };
	uint32_t count = 0;
	if (profile.interactionProfile)
		xrPathToString(instance, profile.interactionProfile, sizeof(profileStr), &count, profileStr);
	Log("%d currentInteractionProfile for right hand %lld %s\n", r, profile.interactionProfile, profileStr);

	for (int i = 0; i < cfgViews.size(); i++)
	{
		glGenFramebuffers(1, &viewsData[i].fbo);
		//glGenRenderbuffers(1, &viewsData[i].rbDepth);

		//glBindRenderbuffer(GL_RENDERBUFFER, viewsData[i].rbDepth);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, viewsData[i].rect.extent.width, viewsData[i].rect.extent.height);

		glBindFramebuffer(GL_FRAMEBUFFER, viewsData[i].fbo);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, viewsData[i].rbDepth);
	}
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	uint32_t cubeVAO;
	uint32_t VAOBuffers[2];
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);
	glGenBuffers(2, VAOBuffers);
	glBindBuffer(GL_ARRAY_BUFFER, VAOBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 8 * 3 * 4, cubeVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VAOBuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24 * 2, cubeInds, GL_STATIC_DRAW);
	//glVertexPointer(3, GL_FLOAT, 12, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 12, 0);
	glBindVertexArray(0);

	std::vector<uint32_t> swapchainsImgIndex(cfgViews.size());
	std::vector<XrView> views(cfgViews.size(), { XR_TYPE_VIEW });
	std::vector<XrCompositionLayerProjectionView> layerProjViews(cfgViews.size());
	XrCompositionLayerProjection layerProjection;
	std::vector<XrCompositionLayerBaseHeader*> frameLayers;
	XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
	uint64_t renderedFrames = 0;
	bool firstTime = true;

	//loop
	bool shouldClose = false;
	while (!shouldClose)
	{
		XrEventDataBuffer event{ XR_TYPE_EVENT_DATA_BUFFER };
		r = xrPollEvent(instance, &event);
		if (r == XR_SUCCESS)
		{
			printf("event type %d %s\n", event.type, XrEnumStr(event.type));
			switch (event.type)
			{
			case XR_TYPE_EVENT_DATA_EVENTS_LOST:
				printf("events lost count %d\n", ((XrEventDataEventsLost*)&event)->lostEventCount);
				break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
				printf("instance loss pending. Time %lld\n", ((XrEventDataInstanceLossPending*)&event)->lossTime);
				shouldClose = true;
				break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
				{
				XrEventDataSessionStateChanged* sessionStateChange = (XrEventDataSessionStateChanged*)&event;
				printf("new session state %d(%s), time %lld\n", sessionStateChange->state, XrEnumStr(sessionStateChange->state), sessionStateChange->time);
				sessionState = sessionStateChange->state;
				if (sessionState == XR_SESSION_STATE_READY)
				{
					XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO,nullptr,viewConfigType};
					XrResult sr = xrBeginSession(session, &sessionBeginInfo);
					if (XR_FAILED(sr))
					{
						CheckXrResult(r, "xrBeginSession");
						shouldClose = true;
					}
				}
				else if (sessionState == XR_SESSION_STATE_STOPPING)
					xrEndSession(session);
				else if (sessionState == XR_SESSION_STATE_EXITING)
					shouldClose = true;

				}
				break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
				{
				const XrEventDataInteractionProfileChanged& e = *(XrEventDataInteractionProfileChanged*)&event;
				XrInteractionProfileState profile{ XR_TYPE_INTERACTION_PROFILE_STATE };
				r = xrGetCurrentInteractionProfile(e.session, handsPaths[1], &profile);
				char profileStr[XR_MAX_PATH_LENGTH]{ 0 };
				uint32_t count = 0;
				if (profile.interactionProfile)
					xrPathToString(instance, profile.interactionProfile, sizeof(profileStr), &count, profileStr);
				Log(" right hand %lld %s\n", profile.interactionProfile, profileStr);
				}
				break;
			}
		}
		else if(r != XR_EVENT_UNAVAILABLE)
		{
			char buffer[XR_MAX_RESULT_STRING_SIZE] = "";
			xrResultToString(XR_NULL_HANDLE, r, buffer);
			printf("xrPollEvent returned %s\n", buffer);
			if (XR_FAILED(r)) {
				shouldClose = true;
				continue;
			}
		}
		
		glfwPollEvents();
		if (glfwWindowShouldClose(window))
			xrRequestExitSession(session);

		if (sessionState == XR_SESSION_STATE_READY || sessionState == XR_SESSION_STATE_SYNCHRONIZED
			|| sessionState == XR_SESSION_STATE_VISIBLE || sessionState == XR_SESSION_STATE_FOCUSED)
		{
			XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO,nullptr};
			XrFrameState frameState{ XR_TYPE_FRAME_STATE,nullptr };
			r = xrWaitFrame(session, &frameWaitInfo, &frameState);
			if (r) Log("%d wait frame: time %lld %lld render %d\n", r, frameState.predictedDisplayTime, frameState.predictedDisplayPeriod, frameState.shouldRender);
			if (r != XR_SUCCESS)
				return r;
			
			XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO,nullptr };
			r = xrBeginFrame(session, &frameBeginInfo);
			if (r) Log("%d begin frame\n", r);
			if (r != XR_SUCCESS)
				return r;

			XrActiveActionSet syncSet{ actionSet,XR_NULL_PATH };
			XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO,nullptr,1,&syncSet };
			xrSyncActions(session,&syncInfo);

			if (frameState.shouldRender)
			{
				frameLayers.clear();

				for (int i = 0; i < viewsData.size(); i++)
				{
					XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
					XrResult ar = xrAcquireSwapchainImage(viewsData[i].swapchain, &acquireInfo, &swapchainsImgIndex[i]);
					if (XR_FAILED(ar))
					{
						char buffer[XR_MAX_RESULT_STRING_SIZE] = "";
						xrResultToString(XR_NULL_HANDLE, ar, buffer);
						printf("xrAcquireSwapchainImage %d failed %s\n", i, buffer);
						return r;
					}
					/*else {
						printf("Swapchain image acquired: %d\n", swapchainImgIndex);
					}*/

					XrSwapchainImageWaitInfo imageWaitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,nullptr, 100000000 };//100ms
					r = xrWaitSwapchainImage(viewsData[i].swapchain, &imageWaitInfo);
					if (r) Log("%d wait image\n", r);
					if (r != XR_SUCCESS)
						return r;
				}

				XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO,nullptr,viewConfigType,frameState.predictedDisplayTime, localSpace };
				XrViewState viewState{ XR_TYPE_VIEW_STATE };
				uint32_t viewCount = 0;
				r = xrLocateViews(session, &viewLocateInfo, &viewState, views.size(), &viewCount, views.data());
				if (r) Log("%d locate views: flags %X. views %d\n", r, viewState.viewStateFlags, count);

				for (size_t i = 0; i < views.size(); i++)
				{
					layerProjViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, nullptr, views[i].pose, views[i].fov };
					layerProjViews[i].subImage = { viewsData[i].swapchain,viewsData[i].rect, 0 };
				}

				XrSpaceLocation spaceLocations[2]{ {XR_TYPE_SPACE_LOCATION} ,{XR_TYPE_SPACE_LOCATION} };
				xrLocateSpace(gripSpaces[0], localSpace, frameState.predictedDisplayTime, spaceLocations);
				xrLocateSpace(gripSpaces[1], localSpace, frameState.predictedDisplayTime, spaceLocations+1);

				//RENDER!!!
				for (int i = 0; i < views.size(); i++)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, viewsData[i].fbo);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewsData[i].images[swapchainsImgIndex[i]].image, 0);
					glViewport(viewsData[i].rect.offset.x, viewsData[i].rect.offset.y, viewsData[i].rect.extent.width, viewsData[i].rect.extent.height);

					//int i = 0;
					//glBindFramebuffer(GL_FRAMEBUFFER, 0);
					//glViewport(0,0,800,480);

					glClearColor(0.3, 0.4, 0.6, 1.0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glm::mat4 vpMtx(1);
					vpMtx = glm::translate(vpMtx, glm::make_vec3(&views[i].pose.position.x));
					vpMtx *= glm::mat4(glm::quat(views[i].pose.orientation.w, views[i].pose.orientation.x, views[i].pose.orientation.y, views[i].pose.orientation.z));
					vpMtx = glm::inverse(vpMtx);
					vpMtx = FrustumXR(views[i].fov, 0.01f, 1000.0f) * vpMtx;

					glm::mat4 mvpMtx = vpMtx;

					simpleShader.Use();
					simpleShader.UniformMat4(simpleShader.u_mvpMtx, &mvpMtx[0].x);

					//glLineWidth(8);

					glBindVertexArray(cubeVAO);
					glDrawElements(GL_LINES,24,GL_UNSIGNED_SHORT,0);

					mvpMtx = vpMtx * glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.2, 0.0, -0.3)), glm::vec3(0.1));
					simpleShader.UniformMat4(simpleShader.u_mvpMtx, &mvpMtx[0].x);
					glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, 0);

					for (int h = 0; h < 2; h++)
					{
						if (spaceLocations[h].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
						{
							mvpMtx = vpMtx * glm::scale(glm::translate(glm::mat4(1), glm::make_vec3(&spaceLocations[h].pose.position.x)) * glm::mat4(glm::make_quat(&spaceLocations[h].pose.orientation.x)), glm::vec3(0.05));
							simpleShader.UniformMat4(simpleShader.u_mvpMtx, &mvpMtx[0].x);
							glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, 0);
						}
					}

					//glfwSwapBuffers(window);
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);


				XrSwapchainImageReleaseInfo imageReleaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr };
				for (int i = 0; i < cfgViews.size(); i++)
				{
					r = xrReleaseSwapchainImage(viewsData[i].swapchain, &imageReleaseInfo);
					if (r) Log("%d release image\n", r);
				}
				
				layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION, nullptr, 0,localSpace,(uint32_t)layerProjViews.size(),layerProjViews.data() };
				frameLayers.push_back((XrCompositionLayerBaseHeader*)&layerProjection);

				renderedFrames++;
			}

			XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO,nullptr,frameState.predictedDisplayTime,XR_ENVIRONMENT_BLEND_MODE_OPAQUE,frameLayers.size(),frameLayers.data()};
			r = xrEndFrame(session, &frameEndInfo);
			if (r) Log("%d end frame\n", r);
			if (r != XR_SUCCESS)
				return r;
		}
	}

	printf("Rendered %lld frames\n",renderedFrames);
	//destroy
	for (size_t i = 0; i < cfgViews.size(); i++)
	{
		xrDestroySwapchain(viewsData[i].swapchain);
	}
	xrDestroySession(session);
	if(have_EXT_debug_utils)
		xrDestroyDebugUtilsMessengerEXTd(debugMessenger);
	xrDestroyInstance(instance);

	glfwTerminate();

	printf("Shutting down\n");
	return 0;
}


const char* glfmt_to_str(int f)
{
	switch (f)
	{
	case GL_RGBA4:
		return "RGBA4";
	case GL_RGB5_A1:
		return "RGB5_A1";
	case GL_RGB565:
		return "RGB565";

	case GL_RG:
		return "RG";

	case GL_R8:
		return "R8";
	case GL_RG8:
		return "RG8";
	case GL_RGB8:
		return "RGB8";
	case GL_RGBA8:
		return "RGBA8";
	case GL_RGB10_A2:
		return "RGB10_A2";

	case GL_R16F:
		return "R16F";
	case GL_RG16F:
		return "RG16F";
	case GL_RGB16F:
		return "RGB16F";
	case GL_RGBA16F:
		return "RGBA16F";

	case GL_R16I:
		return "R16I";
	case GL_RG16I:
		return "RG16I";
	case GL_RGB16I:
		return "RGB16I";
	case GL_RGBA16I:
		return "RGBA16I";
	case GL_R16UI:
		return "R16UI";
	case GL_RG16UI:
		return "RG16UI";
	case GL_RGB16UI:
		return "RGB16UI";
	case GL_RGBA16UI:
		return "RGBA16UI";

	case GL_R8_SNORM:
		return "R8_SNORM";
	case GL_RG8_SNORM:
		return "RG8_SNORM";
	case GL_RGB8_SNORM:
		return "RGB8_SNORM";
	case GL_RGBA8_SNORM:
		return "RGBA8_SNORM";

	case GL_SR8_EXT:
		return "SR8";
	case GL_SRGB:
		return "SRGB";
	case GL_SRGB8:
		return "SRGB8";
	case GL_SRGB8_ALPHA8:
		return "SRGB8_A8";

	case GL_DEPTH_COMPONENT16:
		return "DEPTH16";
	case GL_DEPTH_COMPONENT24:
		return "DEPTH24";
	case GL_DEPTH_COMPONENT32F:
		return "DEPTH32F";
	case GL_DEPTH24_STENCIL8:
		return "DEPTH24_STENCIL8";
	case GL_DEPTH32F_STENCIL8:
		return "DEPTH32F_STENCIL8";

	case GL_R16:
		return "R16";
	case GL_RG16:
		return "RG16";
	case GL_RGB16_EXT:
		return "RGB16";
	case GL_RGBA16_EXT:
		return "RGBA16";
	case GL_R16_SNORM:
		return "R16_SNORM";
	case GL_RG16_SNORM:
		return "RG16_SNORM";
	case GL_RGB16_SNORM:
		return "RGB16_SNORM";
	case GL_RGBA16_SNORM:
		return "RGBA16_SNORM";

		//compressed
	case GL_PALETTE4_RGB8_OES:
		return "PALETTE4_RGB8";
	case GL_PALETTE4_RGBA8_OES:
		return "PALETTE4_RGBA8";
	case GL_PALETTE4_R5_G6_B5_OES:
		return "PALETTE4_R5_G6_B5";
	case GL_PALETTE4_RGBA4_OES:
		return "PALETTE4_RGBA4";
	case GL_PALETTE4_RGB5_A1_OES:
		return "PALETTE4_RGB5_A1";
	case GL_PALETTE8_RGB8_OES:
		return "PALETTE8_RGB8";
	case GL_PALETTE8_RGBA8_OES:
		return "PALETTE8_RGBA8";
	case GL_PALETTE8_R5_G6_B5_OES:
		return "PALETTE8_R5_G6_B5";
	case GL_PALETTE8_RGBA4_OES:
		return "PALETTE8_RGBA4";
	case GL_PALETTE8_RGB5_A1_OES:
		return "PALETTE8_RGB5_A1";
#ifdef GL_OES_compressed_ETC1_RGB8_texture
	case GL_ETC1_RGB8_OES:
		return "ETC1_RGB8";
#endif
	case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
		return "RGBA_ASTC_4x4";
	case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
		return "RGBA_ASTC_5x4";
	case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
		return "RGBA_ASTC_5x5";
	case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
		return "RGBA_ASTC_6x5";
	case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
		return "RGBA_ASTC_6x6";
	case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
		return "RGBA_ASTC_8x5";
	case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
		return "RGBA_ASTC_8x6";
	case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
		return "RGBA_ASTC_8x8";
	case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
		return "RGBA_ASTC_10x5";
	case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
		return "RGBA_ASTC_10x6";
	case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
		return "RGBA_ASTC_10x8";
	case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
		return "RGBA_ASTC_10x10";
	case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
		return "RGBA_ASTC_12x10";
	case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
		return "RGBA_ASTC_12x12";

	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
		return "SRGB8_ALPHA8_ASTC_4x4";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
		return "SRGB8_ALPHA8_ASTC_5x4";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
		return "SRGB8_ALPHA8_ASTC_5x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
		return "SRGB8_ALPHA8_ASTC_6x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
		return "SRGB8_ALPHA8_ASTC_6x6";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
		return "SRGB8_ALPHA8_ASTC_8x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
		return "SRGB8_ALPHA8_ASTC_8x6";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
		return "SRGB8_ALPHA8_ASTC_8x8";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
		return "SRGB8_ALPHA8_ASTC_10x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
		return "SRGB8_ALPHA8_ASTC_10x6";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
		return "SRGB8_ALPHA8_ASTC_10x8";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
		return "SRGB8_ALPHA8_ASTC_10x10";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
		return "SRGB8_ALPHA8_ASTC_12x10";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
		return "SRGB8_ALPHA8_ASTC_12x12";

#ifdef GL_OES_texture_compression_astc
	case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:
		return "RGBA_ASTC_3x3x3";
	case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:
		return "RGBA_ASTC_4x3x3";
	case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:
		return "RGBA_ASTC_4x4x3";
	case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:
		return "RGBA_ASTC_4x4x4";
	case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:
		return "RGBA_ASTC_5x4x4";
	case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:
		return "RGBA_ASTC_5x5x4";
	case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:
		return "RGBA_ASTC_5x5x5";
	case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:
		return "RGBA_ASTC_6x5x5";
	case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:
		return "RGBA_ASTC_6x6x5";
	case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:
		return "RGBA_ASTC_6x6x6";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:
		return "SRGB8_ALPHA8_ASTC_3x3x3";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:
		return "SRGB8_ALPHA8_ASTC_4x3x3";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:
		return "SRGB8_ALPHA8_ASTC_4x4x3";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:
		return "SRGB8_ALPHA8_ASTC_4x4x4";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:
		return "SRGB8_ALPHA8_ASTC_5x4x4";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:
		return "SRGB8_ALPHA8_ASTC_5x5x4";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:
		return "SRGB8_ALPHA8_ASTC_5x5x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:
		return "SRGB8_ALPHA8_ASTC_6x5x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:
		return "SRGB8_ALPHA8_ASTC_6x6x5";
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:
		return "SRGB8_ALPHA8_ASTC_6x6x6";
#endif

	case GL_COMPRESSED_R11_EAC:
		return "R11_EAC";
	case GL_COMPRESSED_SIGNED_R11_EAC:
		return "SIGNED_R11_EAC";
	case GL_COMPRESSED_RG11_EAC:
		return "RG11_EAC";
	case GL_COMPRESSED_SIGNED_RG11_EAC:
		return "SIGNED_RG11_EAC";
	case GL_COMPRESSED_RGB8_ETC2:
		return "RGB8_ETC2";
	case GL_COMPRESSED_SRGB8_ETC2:
		return "SRGB8_ETC2";
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		return "RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		return "SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
		return "RGBA8_ETC2_EAC";
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
		return "SRGB8_ALPHA8_ETC2_EAC";

	default:
		return "<?>";
	}
}
