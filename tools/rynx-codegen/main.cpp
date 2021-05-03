
#include <vector>
#include <string>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <unordered_map>

#include <filesystem>
#include <fstream>

#include <clang-c/Index.h>

std::string getString(CXString str) {
	std::string cStr(clang_getCString(str));
	clang_disposeString(str);
	return cStr;
}

std::string getNamespace(CXCursor cursor) {
	CXCursor semParent = clang_getCursorSemanticParent(cursor);
	auto kind = clang_getCursorKind(semParent);
	
	if (kind == CXCursorKind::CXCursor_FirstInvalid ||
		kind == CXCursorKind::CXCursor_TranslationUnit)
		return {};

	
	if (kind == CXCursorKind::CXCursor_Namespace)
		return getNamespace(semParent) + "::" + getString(clang_getCursorSpelling(semParent));
	return getNamespace(semParent);
}

std::string getFullQualifiedName(CXCursor cursor) {
	CXCursor semParent = clang_getCursorSemanticParent(cursor);
	auto kind = clang_getCursorKind(semParent);
	if (kind == CXCursorKind::CXCursor_FirstInvalid ||
		kind == CXCursorKind::CXCursor_TranslationUnit)
		return {};
	
	std::string currentLayerName = getString(clang_getCursorSpelling(semParent));
	if (currentLayerName.empty())
		return getFullQualifiedName(semParent);
	return getFullQualifiedName(semParent) + "::" + currentLayerName;
}

std::string g_currentlyWorkedOnFile;

bool isCursorFromCurrentFile(CXCursor cursor) {
	CXSourceLocation sourceLoc = clang_getCursorLocation(clang_getCursorDefinition(cursor));

	CXFile sourceF;
	unsigned int a, b, c;
	clang_getFileLocation(sourceLoc, &sourceF, &a, &b, &c);

	CXString fileName = clang_getFileName(sourceF);
	const char* cstr = clang_getCString(fileName);
	if (cstr != nullptr) {
		bool retVal = (g_currentlyWorkedOnFile == cstr);
		std::cout << g_currentlyWorkedOnFile << " vs " << std::string(cstr) << " - " << (retVal ? "true" : "false") << std::endl;
		clang_disposeString(fileName);
		return retVal;
	}
	return false;
}

std::vector<std::string> getAnnotations(CXCursor cursor) {
	std::vector<std::string> output;
	clang_visitChildren(
		cursor,
		[](CXCursor cursor, CXCursor /* parent */, CXClientData client_data) {
			if (clang_getCursorKind(cursor) == CXCursorKind::CXCursor_AnnotateAttr) {
				auto* outputPtr = static_cast<std::vector<std::string>*>(client_data);
				outputPtr->emplace_back(getString(clang_getCursorDisplayName(cursor)));
			}
			return CXChildVisitResult::CXChildVisit_Continue;
		},
		&output
	);
	return output;
}

struct field
{
	bool operator == (const field& other) const = default;

	std::string spelling;
	std::string type;
	std::vector<std::string> annotations;
};

namespace std {
	template<>
	struct hash<field> {
		size_t operator()(const field& f) const {
			return std::hash<std::string>()(f.spelling);
		}
	};
}

struct reflected_type
{
	std::unordered_set<field> m_fields;
	std::vector<std::string> annotations;
	bool generate_reflection = true;
	bool generate_serialization = true;
};

std::vector<std::string> namespaces;
std::vector<char const*> g_clangOptions;
std::unordered_map<std::string, reflected_type> g_reflected_types;

bool handleInterestingCursor(CXCursor cursor, std::string canonicalType = "", bool generate_serialization = true) {
	
	CXCursorKind kind = clang_getCursorKind(cursor);

	if (kind == CXCursorKind::CXCursor_FieldDecl) {
		std::string spelling = getString(clang_getCursorSpelling(cursor));
		std::string displayName = getString(clang_getCursorDisplayName(cursor));

		if (clang_getCXXAccessSpecifier(cursor) != CX_CXXAccessSpecifier::CX_CXXPublic) {
			return false;
		}
		
		CXType def = clang_getCursorType(cursor);
		CXType canonType = clang_getCanonicalType(def);
		std::string field_type_spelling = getString(clang_getTypeSpelling(canonType));
		
		struct userData {
			std::string field_type_spelling;
			bool generate_serialization;
		};

		userData data;
		data.field_type_spelling = field_type_spelling;
		std::cout << "checking: " << getString(clang_getCursorSpelling(clang_getCursorDefinition(cursor))) << std::endl;
		data.generate_serialization = isCursorFromCurrentFile(clang_getTypeDeclaration(clang_getCursorType(cursor)));

		clang_Type_visitFields(
			canonType,
			[](CXCursor fieldCursor, CXClientData client_data) {
				handleInterestingCursor(fieldCursor, static_cast<userData*>(client_data)->field_type_spelling, static_cast<userData*>(client_data)->generate_serialization);
				return CXVisitorResult::CXVisit_Continue;
			},
			&data
		);
		
		field currentField;
		currentField.spelling = spelling;
		currentField.type = field_type_spelling;
		currentField.annotations = getAnnotations(cursor);

		if (canonicalType.empty()) {
			std::string fullTypeName = getFullQualifiedName(cursor);
			g_reflected_types[fullTypeName].m_fields.emplace(currentField);
		}
		else {
			g_reflected_types["::" + canonicalType].m_fields.emplace(currentField);
			g_reflected_types["::" + canonicalType].generate_serialization = generate_serialization;
		}
	}

	if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl) {
		std::string spelling = getString(clang_getCursorSpelling(cursor));
		std::string targetTypeName = getString(clang_getTypeSpelling(clang_getCursorType(cursor)));
		if (!canonicalType.empty()) {
			targetTypeName = canonicalType;
		}

		// serialization only generated for types declared in processed file.
		if (canonicalType.empty()) {
			std::cout << "canonType: " << canonicalType << ", targetType: " << targetTypeName << std::endl;
			g_reflected_types["::" + targetTypeName].generate_serialization = isCursorFromCurrentFile(clang_getCursorDefinition(cursor));
		}

		// handle inheritance - visit through base classes recursively.
		clang_visitChildren(
			cursor,
			[](CXCursor child, CXCursor /* parent */, CXClientData data) {
				if (clang_getCursorKind(child) == CXCursorKind::CXCursor_CXXBaseSpecifier) {
					CXType childType = clang_getCursorType(child);
					
					// call once with the base type - to enable recursive base class crawling.
					handleInterestingCursor(clang_getTypeDeclaration(childType), *static_cast<std::string*>(data));
					
					// and call once for each field to attach base class public members to derived class reflection.
					clang_Type_visitFields(
						childType,
						[](CXCursor fieldCursor, CXClientData client_data) {
							handleInterestingCursor(fieldCursor, *static_cast<std::string*>(client_data));
							return CXVisitorResult::CXVisit_Continue;
						},
						data
					);
				}
				return CXChildVisit_Continue;
			},
			&targetTypeName
		);

		CXType type = clang_getCursorType(cursor);
		auto typeSizeOf = clang_Type_getSizeOf(type);
		if (typeSizeOf == CXTypeLayoutError_Incomplete || typeSizeOf == CXTypeLayoutError_Dependent) {
			return false;
		}

		if (!spelling.empty()) {
			// ensures the type is created (operator []), and also stores annotations if present.
			g_reflected_types[getFullQualifiedName(cursor) + "::" + spelling].annotations = getAnnotations(cursor);
		}
	}

	return false;
}

void compile(CXIndex& index, std::string path);

std::vector<std::string> g_generated_reflection_functions;

void write_results(std::string source_file) {
	{
		std::string templateTypeNamePrefix("type-parameter");
		std::vector<std::string> templateTypes;
		for (auto&& type : g_reflected_types) {
			for (auto&& field : type.second.m_fields) {
				if (field.type.length() > templateTypeNamePrefix.length() && field.type.find(templateTypeNamePrefix) != std::string::npos) {
					templateTypes.emplace_back(type.first);
					break;
				}
			}
		}

		for (auto&& typeName : templateTypes) {
			std::cout << "erasing incomplete template type " << typeName << std::endl;
			g_reflected_types.erase(typeName);
		}

		std::string standardTypePrefix("::std::");
		std::vector<std::string> standardTypes;
		for (auto&& type : g_reflected_types) {
			if (type.first.length() > standardTypePrefix.length() && memcmp(type.first.data(), standardTypePrefix.data(), standardTypePrefix.length()) == 0) {
				standardTypes.emplace_back(type.first);
			}
		}

		for (auto&& typeName : standardTypes) {
			std::cout << "erasing standard type " << typeName << std::endl;
			g_reflected_types.erase(typeName);
		}
	}

	std::string includeSourceFile = source_file;

	// format processed header file paths
	{
		// for (auto&& path : compiledHeaderFiles) {
			for (auto& c : source_file) {
				if (c == '\\')
					c = '/';
			}

			while (source_file.front() == '.' || source_file.front() == '/') {
				source_file.erase(source_file.begin());
			}

			auto pos = source_file.find(std::string("src/"));
			if (pos != std::string::npos) {
				includeSourceFile = source_file.substr(pos + 4);
			}
		// }
	}


	std::string pathBase = source_file.substr(0, source_file.size() - 4);
	std::string functionRandomName = "register_generated_reflections_";

	{
		std::string randomPartOfName = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + "_" + std::to_string(rand());
		for (auto& c : randomPartOfName) {
			if (c >= '0' && c <= '9') {
				c -= '0';
				c += 'a';
			}
		}
		functionRandomName += randomPartOfName;
	}
	
	g_generated_reflection_functions.emplace_back(functionRandomName);

	// write reflection hpp
	{
		std::cout << "writing '" << pathBase + "_reflection.hpp" << "'" << std::endl;
		std::ofstream fileOutput(pathBase + "_reflection.hpp");
		std::ostream& output = fileOutput;

		output << std::endl << "// This is a generated file. Not productive to modify by hand." << std::endl;
		output << std::endl;
		output << "namespace rynx {" << std::endl;
		output << "namespace reflection {" << std::endl;
		output << "class reflections;" << std::endl;
		output << "namespace generated {" << std::endl;
		output << "void " << functionRandomName << "(rynx::reflection::reflections& type_reflections);" << std::endl;
		output << "extern int " << functionRandomName << "_i;" << std::endl;
		output << "}" << std::endl;
		output << "}" << std::endl;
		output << "}" << std::endl;
	}

	// write reflection cpp
	{
		std::cout << "writing '" << pathBase + "_reflection.cpp" << "'" << std::endl;
		std::ofstream fileOutput(pathBase + "_reflection.cpp");
		std::ostream& output = fileOutput;

		{
			output << std::endl << "// This is a generated file. Not productive to modify by hand." << std::endl;

			// include processed files.
			output << "#include <" << (includeSourceFile.substr(0, includeSourceFile.size() - 4) + "_reflection.hpp>") << std::endl;
			// for (auto&& path : compiledHeaderFiles) {
			output << "#include <" << includeSourceFile << ">" << std::endl;
			// }
			output << "#include <rynx/tech/reflection.hpp>" << std::endl;

			output << std::endl;
			output << "namespace rynx {" << std::endl;
			output << "namespace reflection {" << std::endl;
			output << "namespace generated {" << std::endl;

			output << std::endl << "void " << functionRandomName << "(rynx::reflection::reflections& type_reflections) {" << std::endl;

			for (auto&& type : g_reflected_types) {
				if (!type.second.generate_reflection)
					continue;

				output << "{" << std::endl;
				if (type.second.m_fields.empty()) {
					output << "\t" << "type_reflections.create<" << type.first << ">();" << std::endl;
				}
				else {
					output << "\t" << "auto& reflection = type_reflections.create<" << type.first << ">();" << std::endl;
				}

				for (auto&& field : type.second.m_fields) {
					output << "\t" << "reflection.add_field(\"" << field.spelling << "\", &" << type.first << "::" << field.spelling;
					if (!field.annotations.empty()) {
						output << ", std::vector<std::string>{\n\t\t\"" << field.annotations.front() << "\"";
						for (int annotation_i = 1; annotation_i < field.annotations.size(); ++annotation_i) {
							output << ",\n\t\t\"" << field.annotations[annotation_i] << "\"";
						}
						output << "\n\t}";
					}

					output << ");" << std::endl;
				}
				output << "}" << std::endl << std::endl;
			}

			output << std::endl << "} // reflection register func" << std::endl;

			output << "rynx::reflection::internal::registration_object " << functionRandomName << "_loader([](rynx::reflection::reflections& type_reflections) { " << functionRandomName << "(type_reflections); });" << std::endl;
			output << "int " << functionRandomName << "_i = 0;" << std::endl;

			output
				<< std::endl << "} // namespace generated"
				<< std::endl << "} // namespace reflection"
				<< std::endl << "} // namespace rynx"
				<< std::endl;
		}
	}

	// write serialization hpp
	{
		std::cout << "writing '" << pathBase + "_serialization.hpp" << "'" << std::endl;
		std::ofstream fileOutput(pathBase + "_serialization.hpp");
		std::ostream& output = fileOutput;

		bool compiler_is_forced_to_evaluate_reflection_loader_obj = false;
		
		{
			output << std::endl << "// This is a generated file. Not productive to modify by hand." << std::endl;

			// include processed files.
			// for (auto&& path : compiledHeaderFiles) {
				output << "#include <" << includeSourceFile << ">" << std::endl;
			// }
				output << "#include <rynx/tech/serialization.hpp>" << std::endl;

			output << std::endl;
			output << "namespace rynx {" << std::endl;
			output << "namespace reflection { namespace generated { ";
			output << "extern int " << (functionRandomName + "_i;");
			output << "}}" << std::endl;


			output << "namespace serialization {" << std::endl;
			
			for (auto&& type : g_reflected_types) {
				bool skipType = false;
				for (auto&& ann : type.second.annotations)
					skipType |= (ann == "transient");

				if (skipType)
					continue;

				if (!type.second.generate_serialization) {
					std::cout << "serialization not generated for " << type.first << std::endl;
					continue;
				}

				output << "template<> struct Serialize<" + type.first + "> {\n";
				output << "\ttemplate<typename IOStream> static void serialize(const " + type.first + "& t, IOStream& writer) {\n";
				for (auto&& field : type.second.m_fields) {
					if (true || !compiler_is_forced_to_evaluate_reflection_loader_obj) {
						output << "\t\t++rynx::reflection::generated::" << (functionRandomName + "_i;\n");
						compiler_is_forced_to_evaluate_reflection_loader_obj = true;
					}
					output << "\t\t" << "rynx::serialize(t." << field.spelling << ", writer);\n";
				}
				output << "\t}" << std::endl;

				output << "\ttemplate<typename IOStream> static void deserialize(" + type.first + "& t, IOStream& reader) {\n";
				for (auto&& field : type.second.m_fields) {
					output << "\t\t" << "rynx::deserialize(t." << field.spelling << ", reader);\n";
				}
				output << "\t}" << std::endl;

				output << "};" << std::endl << std::endl;


				// also write definition for "for_each_id_field"
				bool is_id_type = (type.first == "::rynx::ecs::id" || type.first == "::rynx::id" || type.first == "::rynx::ecs_internal::id");

				// only write the definitions if this type is not an id type, and has some fields.
				if (!type.second.m_fields.empty() && !is_id_type) {
					output << "template<> struct for_each_id_field_t<" << type.first << "> {\n";
					output << "\ttemplate<typename Op> static void execute(" << type.first << "& t, Op&& op) {\n";
					if (!compiler_is_forced_to_evaluate_reflection_loader_obj) {
						output << "\t\t++rynx::reflection::generated::" << (functionRandomName + "_i;\n");
						compiler_is_forced_to_evaluate_reflection_loader_obj = true;
					}
					for (auto&& field : type.second.m_fields) {
						output << "\t\trynx::for_each_id_field(t." << field.spelling << ", std::forward<Op>(op));\n";
					}
					output << "\t}" << std::endl; // end function
					output << "};" << std::endl << std::endl; // end struct
				}
			}

			output << std::endl << "} // namespace serialization"
				<< std::endl << "} // namespace rynx"
				<< std::endl;
		}
	}
}

int main(int argc, char** argv) {
	std::vector<std::string> fileMatchers;
	std::vector<std::string> fileEnds;
	std::vector<std::string> compiledHeaderFiles;

	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "-filename-contains") {
			fileMatchers.emplace_back(std::string(argv[++i]));
		}
		else if (std::string(argv[i]) == "-filename-ends-with") {
			fileEnds.emplace_back(std::string(argv[++i]));
		}
		else if (std::string(argv[i]) == "-gen-for-namespace") {
			namespaces.emplace_back(std::string(argv[++i]));
		}
		else if (std::string(argv[i]) == "-I") {
			g_clangOptions.emplace_back(argv[i]);
			g_clangOptions.emplace_back(argv[++i]);
		}
		else if (std::string(argv[i]).starts_with("-D")) {
			g_clangOptions.emplace_back(argv[i]);
			g_clangOptions.emplace_back(argv[++i]);
		}
		else if (std::string(argv[i]).find(std::string("--std")) != std::string::npos) {
			g_clangOptions.emplace_back(argv[i]);
		}
		else {
			std::cerr << "unrecognized option '" << std::string(argv[i]) << "', commands are:\n"
				"-filename-contains <partial matching string, for example \"-filename-contains components\">\n"
				"-filename-ends-with <only concider files that end with this string. for example cpp>\n"
				"-gen-for-namespace ::namespace::to::gen\n"
				"-I <includepath>\n"
				"-Dmydefine\n"
				"--std=desiredstandard" << std::endl;
			return 0;
		}
	}

	g_clangOptions.emplace_back("-DRYNX_CODEGEN");

	CXIndex index = clang_createIndex(0, 0);
	std::filesystem::path wd_path(".");
	std::filesystem::directory_entry wd(wd_path);

	for (auto& p : std::filesystem::recursive_directory_iterator(".")) {
		if (p.is_regular_file()) {
			std::string path_string = p.path().string();
			
			bool matchesAny = fileMatchers.empty();
			for (auto&& matcher : fileMatchers)
				matchesAny |= (path_string.find(matcher) != std::string::npos);

			bool endsWithMatch = fileEnds.empty();
			for (auto&& fileEnd : fileEnds)
				endsWithMatch |= path_string.ends_with(fileEnd);
			
			if (matchesAny && endsWithMatch) {
				g_reflected_types.clear();

				compiledHeaderFiles.emplace_back(path_string);
				compile(index, path_string);

				write_results(path_string);
			}
		}
	}

	clang_disposeIndex(index);
	return 0;
}

void compile(CXIndex& index, std::string path) {
	std::cout << "processing " << path << " ..." << std::endl;
	
	CXTranslationUnit unit;
	[[maybe_unused]] CXErrorCode error = clang_parseTranslationUnit2(index, path.c_str(), g_clangOptions.data(), int(g_clangOptions.size()), nullptr, 0, 0, &unit);

	std::vector<std::string> includeNamespace = {""};
	g_currentlyWorkedOnFile = path;

	CXCursor cursor = clang_getTranslationUnitCursor(unit);
	clang_visitChildren(
		cursor,
		[](CXCursor cursor, CXCursor /* parent */, CXClientData /* client_data */) {
			std::string fullNamespace = getNamespace(cursor);
			bool accept = namespaces.empty();
			for (auto&& ns_name : namespaces) {
				accept |= (fullNamespace.find(ns_name) != std::string::npos);
			}

			if (accept) {
				if (handleInterestingCursor(cursor))
					CXChildVisitResult::CXChildVisit_Continue;
			}
			return CXChildVisitResult::CXChildVisit_Recurse;
		},
		nullptr
	);

	clang_disposeTranslationUnit(unit);

	for (auto&& type : g_reflected_types) {
		if (type.second.m_fields.empty()) {
			type.second.generate_serialization = false;
		}
	}
}
