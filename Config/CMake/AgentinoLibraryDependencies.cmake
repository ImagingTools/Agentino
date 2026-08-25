# ---------------------------------------------------------------------------
# Clean, target-based inter-library dependency graph for Agentino.
#
# Instead of relying on the
# final executable/package link to resolve symbols and on a hand-tuned build
# order (the add_dependencies()/inline target_link_libraries() spread across the
# per-library CMake files), the dependencies between the Agentino libraries - and
# their dependencies onto the underlying ImtCore::, Acf::, AcfSln:: and IAcf::
# libraries - are declared here as target usage requirements. Include paths and
# link order then propagate transitively and automatically for the in-tree build.
#
# Every ImtCore::/Acf::/AcfSln:: imported target exposes its whole source include
# tree (acf_register_library() adds INCLUDE_DIR/IMPL_DIR as PUBLIC include
# directories), so a single ImtCore:: dependency transitively provides the full
# ImtCore, Acf, AcfSln and IAcf header search paths to the consuming library.
#
# Link scopes are explicit in this file (PUBLIC/PRIVATE/INTERFACE) and applied
# through declare_target_dependencies() from
# ACF/Acf/Config/CMake/ProjectRoot.cmake.
# The helper uses target properties to preserve modern scope semantics without
# plain-vs-keyword target_link_libraries() conflicts.
#
# Dependencies are declared *minimally*: each library lists only its direct
# dependencies; transitive dependencies propagate automatically through the graph.
# Do not add a dependency that is already reachable through another listed target.
#
# Included centrally from Build/CMake/CMakeLists.txt.
# When AGENTINO_DECLARE_DEPENDENCIES_HELPER_ONLY is ON, only the helper
# function is defined (for per-target CMakeLists). The full dependency
# declarations are applied by a later include, after all library targets
# have been created.
# ---------------------------------------------------------------------------

# --- SDL generated libraries ------------------------------------------------
# Agentino's SDL is GraphQL-oriented, so keep imtgql explicit on the local SDL
# root instead of mutating ImtCore::imtbasesdl from a downstream repository.
declare_target_dependencies(agentinosdl		LINK_SCOPE PUBLIC	ImtCore::imtbasesdl ImtCore::imtgql)

# --- Libraries --------------------------------------------------------------
declare_target_dependencies(agentinodata		LINK_SCOPE PUBLIC	agentinosdl ImtCore::imtservice)
declare_target_dependencies(agentgql			LINK_SCOPE PUBLIC	agentinodata agentinosdl ImtCore::imtguigql Qt${QT_VERSION_MAJOR}::WebSockets)
declare_target_dependencies(agentinogql		LINK_SCOPE PUBLIC	agentgql agentinodata)

# --- QML web-resource libraries ---------------------------------------------
if(QT_VERSION_MAJOR EQUAL 6)
	declare_target_dependencies(agentinoqml	LINK_SCOPE PUBLIC	Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

# --- Arxc-generated static libraries ----------------------------------------
declare_target_dependencies(AgentinoLoc		LINK_SCOPE PUBLIC	Acf::icomp)
