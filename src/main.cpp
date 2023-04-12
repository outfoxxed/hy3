#include <src/plugins/PluginAPI.hpp>

#include "Hy3Layout.hpp"
#include "src/Compositor.hpp"

inline HANDLE PHANDLE = nullptr;
inline std::unique_ptr<Hy3Layout> g_Hy3Layout;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
	return HYPRLAND_API_VERSION;
}

void splith(std::string) {
	SLayoutMessageHeader header;
	header.pWindow = g_pCompositor->m_pLastWindow;

	if (!header.pWindow) return;

	const auto workspace = g_pCompositor->getWorkspaceByID(header.pWindow->m_iWorkspaceID);
	if (workspace->m_bHasFullscreenWindow) return;

	g_pLayoutManager->getCurrentLayout()->layoutMessage(header, "splith");
}

void splitv(std::string) {
	SLayoutMessageHeader header;
	header.pWindow = g_pCompositor->m_pLastWindow;

	if (!header.pWindow) return;

	const auto workspace = g_pCompositor->getWorkspaceByID(header.pWindow->m_iWorkspaceID);
	if (workspace->m_bHasFullscreenWindow) return;

	g_pLayoutManager->getCurrentLayout()->layoutMessage(header, "splitv");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

	g_Hy3Layout = std::make_unique<Hy3Layout>();
	HyprlandAPI::addLayout(PHANDLE, "hy3", g_Hy3Layout.get());

	HyprlandAPI::addDispatcher(PHANDLE, "splith", splith);
	HyprlandAPI::addDispatcher(PHANDLE, "splitv", splitv);

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
