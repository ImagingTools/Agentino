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
# through acf_declare_target_dependencies() from
# ACF/Acf/Config/CMake/ProjectRoot.cmake.
# The helper uses target properties to preserve modern scope semantics without
# plain-vs-keyword target_link_libraries() conflicts.
#
# Dependencies are declared *minimally*: each library lists only its direct
# dependencies; transitive dependencies propagate automatically through the graph.
# Do not add a dependency that is already reachable through another listed target.
#
# Included once, centrally, from Build/CMake/CMakeLists.txt after all library
# targets have been created.
# ---------------------------------------------------------------------------

# ImtCore's SDL base library only carries the imtgql usage requirement for
# consumers that opt into it. Agentino's SDL is GraphQL-oriented, so expose
# imtgql through imtbasesdl for every Agentino library that builds on the SDL.
acf_declare_target_dependencies(ImtCore::imtbasesdl	LINK_SCOPE INTERFACE	ImtCore::imtgql)


# --- SDL generated libraries ------------------------------------------------
acf_declare_target_dependencies(agentinosdl		LINK_SCOPE PUBLIC	ImtCore::imtbasesdl)

# --- Libraries --------------------------------------------------------------
acf_declare_target_dependencies(agentinodata		LINK_SCOPE PUBLIC	agentinosdl ImtCore::imtservice)
acf_declare_target_dependencies(agentgql			LINK_SCOPE PUBLIC	agentinodata agentinosdl ImtCore::imtguigql Qt${QT_VERSION_MAJOR}::WebSockets)
acf_declare_target_dependencies(agentinogql		LINK_SCOPE PUBLIC	agentgql agentinodata)

# --- QML web-resource libraries ---------------------------------------------
if(QT_VERSION_MAJOR EQUAL 6)
	acf_declare_target_dependencies(agentinoqml	LINK_SCOPE PUBLIC	Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

# --- Arxc-generated static libraries ----------------------------------------
acf_declare_target_dependencies(AgentinoLoc		LINK_SCOPE PUBLIC	Acf::icomp)
