# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Consolidated single atlas library - contains all functionality
add_library(atlas_lib STATIC
    # Application layer
    StrongTypeGenerator.cpp
    StrongTypeGenerator.hpp
    InteractionGenerator.cpp
    InteractionGenerator.hpp
    AtlasUtilities.cpp
    AtlasUtilities.hpp
    AtlasParser.cpp
    AtlasParser.hpp
    AtlasCommandLine.cpp
    AtlasCommandLine.hpp
    ProfileSystem.cpp
    ProfileSystem.hpp
    TemplateSystem.cpp
    TemplateSystem.hpp
    AtlasMain.cpp
    AtlasMain.hpp

    # Utility layer
    SHA1Hasher.cpp
    SHA1Hasher.hpp
    TypeTokenizer.cpp
    TypeTokenizer.hpp
    TokenToHeaderMapper.cpp
    TokenToHeaderMapper.hpp

    # Generation core
    generation/core/ClassInfo.cpp
    generation/core/ClassInfo.hpp
    generation/core/GuardGenerator.cpp
    generation/core/GuardGenerator.hpp
    generation/core/MainTemplate.cpp
    generation/core/MainTemplate.hpp
    generation/core/TemplateOrchestrator.cpp
    generation/core/TemplateOrchestrator.hpp
    generation/core/TemplateRegistry.cpp
    generation/core/TemplateRegistry.hpp
    generation/core/ITemplate.cpp
    generation/core/ITemplate.hpp

    # Parsing
    generation/parsing/OperatorParser.cpp
    generation/parsing/OperatorParser.hpp

    # Operators - arithmetic
    generation/operators/arithmetic/BinaryOperatorHelpers.cpp
    generation/operators/arithmetic/BinaryOperatorHelpers.hpp
    generation/operators/arithmetic/AdditionOperator.cpp
    generation/operators/arithmetic/AdditionOperator.hpp
    generation/operators/arithmetic/SubtractionOperator.cpp
    generation/operators/arithmetic/SubtractionOperator.hpp
    generation/operators/arithmetic/MultiplicationOperator.cpp
    generation/operators/arithmetic/MultiplicationOperator.hpp
    generation/operators/arithmetic/DivisionOperator.cpp
    generation/operators/arithmetic/DivisionOperator.hpp
    generation/operators/arithmetic/ModuloOperator.cpp
    generation/operators/arithmetic/ModuloOperator.hpp
    generation/operators/arithmetic/UnaryOperators.cpp
    generation/operators/arithmetic/UnaryOperators.hpp
    generation/operators/arithmetic/IncrementOperators.cpp
    generation/operators/arithmetic/IncrementOperators.hpp

    # Operators - bitwise
    generation/operators/bitwise/BitwiseOperators.cpp
    generation/operators/bitwise/BitwiseOperators.hpp

    # Operators - comparison
    generation/operators/comparison/RelationalOperator.cpp
    generation/operators/comparison/RelationalOperator.hpp
    generation/operators/comparison/SpaceshipOperator.cpp
    generation/operators/comparison/SpaceshipOperator.hpp
    generation/operators/comparison/DefaultedEqualityOperator.cpp
    generation/operators/comparison/DefaultedEqualityOperator.hpp

    # Operators - access
    generation/operators/access/ArrowOperator.cpp
    generation/operators/access/ArrowOperator.hpp
    generation/operators/access/IndirectionOperator.cpp
    generation/operators/access/IndirectionOperator.hpp

    # Operators - logical
    generation/operators/logical/BinaryLogicalOperatorHelpers.cpp
    generation/operators/logical/BinaryLogicalOperatorHelpers.hpp
    generation/operators/logical/LogicalNotOperator.cpp
    generation/operators/logical/LogicalNotOperator.hpp
    generation/operators/logical/LogicalAndOperator.cpp
    generation/operators/logical/LogicalAndOperator.hpp
    generation/operators/logical/LogicalOrOperator.cpp
    generation/operators/logical/LogicalOrOperator.hpp

    # Operators - I/O
    generation/operators/io/OStreamOperator.cpp
    generation/operators/io/OStreamOperator.hpp
    generation/operators/io/IStreamOperator.cpp
    generation/operators/io/IStreamOperator.hpp

    # Operators - functional
    generation/operators/functional/NullaryOperator.cpp
    generation/operators/functional/NullaryOperator.hpp
    generation/operators/functional/CallableOperator.cpp
    generation/operators/functional/CallableOperator.hpp
    generation/operators/functional/SubscriptOperator.cpp
    generation/operators/functional/SubscriptOperator.hpp
    generation/operators/functional/AddressOfOperator.cpp
    generation/operators/functional/AddressOfOperator.hpp

    # Operators - conversion
    generation/operators/conversion/BoolOperator.cpp
    generation/operators/conversion/BoolOperator.hpp
    generation/operators/conversion/ExplicitCastOperator.cpp
    generation/operators/conversion/ExplicitCastOperator.hpp
    generation/operators/conversion/ImplicitCastOperator.cpp
    generation/operators/conversion/ImplicitCastOperator.hpp

    # Specializations
    generation/specializations/HashSpecialization.cpp
    generation/specializations/HashSpecialization.hpp
    generation/specializations/FormatterSpecialization.cpp
    generation/specializations/FormatterSpecialization.hpp

    # Features
    generation/features/ConstantTemplate.cpp
    generation/features/ConstantTemplate.hpp
    generation/features/TemplateAssignmentOperator.cpp
    generation/features/TemplateAssignmentOperator.hpp
    generation/features/IteratorSupportTemplate.cpp
    generation/features/IteratorSupportTemplate.hpp
    generation/features/ForwardedMemfnTemplate.cpp
    generation/features/ForwardedMemfnTemplate.hpp)

# Include directories
target_include_directories(atlas_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Link libraries
target_link_libraries(atlas_lib
    PUBLIC
    PRIVATE
        Boost::describe
        Boost::json
        Boost::uuid)

# Apply compiler warnings
target_apply_atlas_warnings(atlas_lib)

# Add boost and mustache include directories
target_include_directories(atlas_lib
    SYSTEM
    PRIVATE
        ${boost_mustache_SOURCE_DIR}/include
        ${boost_SOURCE_DIR})

# Add mustache source file to the library
target_sources(atlas_lib
    PRIVATE
        ${boost_mustache_SOURCE_DIR}/src/renderer.cpp)

# Disable all warnings for third-party mustache source
set_source_files_properties(${boost_mustache_SOURCE_DIR}/src/renderer.cpp
    PROPERTIES COMPILE_FLAGS "-w")

# Force-link all object files in atlas_lib to ensure static initializers run
# This is critical for template self-registration to work correctly.
# By setting this at the library level, any target linking atlas_lib automatically
# gets the whole-archive behavior without needing to repeat it.
if(APPLE)
    # macOS: Use -force_load
    target_link_options(atlas_lib
        INTERFACE
            "LINKER:-force_load,$<TARGET_FILE:atlas_lib>")
elseif(UNIX)
    # Linux: Use --whole-archive
    target_link_options(atlas_lib
        INTERFACE
            "LINKER:--whole-archive"
            "$<TARGET_FILE:atlas_lib>"
            "LINKER:--no-whole-archive")
elseif(MSVC)
    # Windows: Use /WHOLEARCHIVE
    target_link_options(atlas_lib
        INTERFACE
            "/WHOLEARCHIVE:$<TARGET_FILE:atlas_lib>")
endif()
