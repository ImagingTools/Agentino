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
# Link scopes are explicit in this file (PUBLIC/PRIVATE/INTERFACE), and
# dependencies are declared with direct target_link_libraries() calls.
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
if(TARGET ImtCore::imtbasesdl AND TARGET ImtCore::imtgql)
	# CMake forbids target_link_libraries() on ALIAS targets.
	get_target_property(_agentino_imtbasesdl_aliased ImtCore::imtbasesdl ALIASED_TARGET)
	if(NOT _agentino_imtbasesdl_aliased)
		target_link_libraries(ImtCore::imtbasesdl INTERFACE ImtCore::imtgql)
	endif()
endif()


# --- SDL generated libraries ------------------------------------------------
if(TARGET agentinosdl)
	target_link_libraries(agentinosdl PUBLIC ImtCore::imtbasesdl)
endif()

# --- Libraries --------------------------------------------------------------
if(TARGET agentinodata)
	target_link_libraries(agentinodata PUBLIC agentinosdl ImtCore::imtservice)
endif()
if(TARGET agentgql)
	target_link_libraries(agentgql PUBLIC agentinodata agentinosdl ImtCore::imtguigql Qt${QT_VERSION_MAJOR}::WebSockets)
endif()
if(TARGET agentinogql)
	target_link_libraries(agentinogql PUBLIC agentgql agentinodata)
endif()

# --- QML web-resource libraries ---------------------------------------------
if(QT_VERSION_MAJOR EQUAL 6)
	if(TARGET agentinoqml)
		target_link_libraries(agentinoqml PUBLIC Qt${QT_VERSION_MAJOR}::Core5Compat)
	endif()
endif()

# --- Arxc-generated static libraries ----------------------------------------
if(TARGET AgentinoLoc)
	target_link_libraries(AgentinoLoc PUBLIC Acf::icomp)
endif()
