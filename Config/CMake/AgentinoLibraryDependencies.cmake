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
# Agentino uses keyword target_link_libraries() signatures (PUBLIC/PRIVATE/
# INTERFACE) consistently via ACF_LIBRARY_LINK_SCOPE. For static libraries the
# dependency still propagates transitively to consumers.
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

# Declare the dependencies of an Agentino library, ignoring any entry whose
# target does not exist in the current configuration (for example feature-gated
# libraries, or ImtCore::/Acf::/AcfSln::/IAcf:: targets that are not available).
if(NOT COMMAND agentino_declare_library_dependencies)
	function(agentino_declare_library_dependencies target)
		cmake_parse_arguments(ARG "" "LINK_SCOPE" "" ${ARGN})

		if(NOT ARG_LINK_SCOPE)
			set(ARG_LINK_SCOPE ${ACF_LIBRARY_LINK_SCOPE})
		endif()

		if(NOT TARGET ${target})
			return()
		endif()

		# The only entry whose *target* is an ImtCore:: name is the imtbasesdl->imtgql
		# usage-requirement augmentation, needed solely for the imported
		# find_package(ImtCore) target. In a unified in-tree build ImtCore::imtbasesdl is
		# an ALIAS: target_link_libraries() is illegal on it, and augmenting the real
		# target injects a dependency cycle through the Qt autogen targets. Skip aliases.
		get_target_property(_agentino_aliased ${target} ALIASED_TARGET)
		if(_agentino_aliased)
			return()
		endif()

		# Some external ACF templates can still call plain target_link_libraries()
		# on a target before this helper runs. To keep a strict modern scope model
		# without reintroducing legacy mode switches, apply dependencies via target
		# properties instead of a second target_link_libraries() call.
		string(TOUPPER "${ARG_LINK_SCOPE}" _agentino_link_scope)
		get_target_property(_agentino_imported ${target} IMPORTED)
		get_target_property(_agentino_type ${target} TYPE)

		foreach(dependency IN LISTS ARG_UNPARSED_ARGUMENTS)
			if(TARGET ${dependency})
				if(_agentino_imported)
					set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${dependency})
				elseif(_agentino_link_scope STREQUAL "INTERFACE")
					set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${dependency})
				elseif(_agentino_link_scope STREQUAL "PUBLIC")
					set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES ${dependency})
					if(NOT _agentino_type STREQUAL "EXECUTABLE")
						set_property(TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${dependency})
					endif()
				else()
					set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES ${dependency})
				endif()
			endif()
		endforeach()
	endfunction()
endif()

# Allow early include from Build/CMake/CMakeLists.txt to expose helper only.
if(AGENTINO_DECLARE_DEPENDENCIES_HELPER_ONLY)
	return()
endif()

# ImtCore's SDL base library only carries the imtgql usage requirement for
# consumers that opt into it. Agentino's SDL is GraphQL-oriented, so expose
# imtgql through imtbasesdl for every Agentino library that builds on the SDL.
agentino_declare_library_dependencies(ImtCore::imtbasesdl	LINK_SCOPE INTERFACE	ImtCore::imtgql)


# --- SDL generated libraries ------------------------------------------------
agentino_declare_library_dependencies(agentinosdl		LINK_SCOPE PUBLIC	ImtCore::imtbasesdl)

# --- Libraries --------------------------------------------------------------
agentino_declare_library_dependencies(agentinodata		LINK_SCOPE PUBLIC	agentinosdl ImtCore::imtservice)
agentino_declare_library_dependencies(agentgql			LINK_SCOPE PUBLIC	agentinodata agentinosdl ImtCore::imtguigql Qt${QT_VERSION_MAJOR}::WebSockets)
agentino_declare_library_dependencies(agentinogql		LINK_SCOPE PUBLIC	agentgql agentinodata)

# --- QML web-resource libraries ---------------------------------------------
if(QT_VERSION_MAJOR EQUAL 6)
	agentino_declare_library_dependencies(agentinoqml	LINK_SCOPE PUBLIC	Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

# --- Arxc-generated static libraries ----------------------------------------
agentino_declare_library_dependencies(AgentinoLoc		LINK_SCOPE PUBLIC	Acf::icomp)
