#include "Model.h"

void Model::loadModel(const std::string& path) {
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                   aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    // data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        } else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++) // face一般是三角形，所以是3
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<std::shared_ptr<Texture>> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, DIFFUSE_NAME);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    std::vector<std::shared_ptr<Texture>> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, SPECULAR_NAME);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<std::shared_ptr<Texture>> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, NORMAL_NAME);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<std::shared_ptr<Texture>> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, HEIGHT_NAME);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // add PBR textures
    // 1. albedo maps
    std::vector<std::shared_ptr<Texture>> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, PBR_ALBEDO_NAME);
    textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
    // 2. normal maps
    std::vector<std::shared_ptr<Texture>> normalMapsPBR = loadMaterialTextures(material, aiTextureType_NORMAL_CAMERA, PBR_NORMAL_NAME);
    textures.insert(textures.end(), normalMapsPBR.begin(), normalMapsPBR.end());
    // 3. metallic maps
    std::vector<std::shared_ptr<Texture>> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, PBR_METALLIC_NAME);
    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
    // 4. roughness maps
    std::vector<std::shared_ptr<Texture>> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, PBR_ROUGHNESS_NAME);
    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
    // 5. ao maps
    std::vector<std::shared_ptr<Texture>> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, PBR_AO_NAME);
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    // 6. emission maps
    std::vector<std::shared_ptr<Texture>> emissionMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, PBR_EMISSION_NAME);
    textures.insert(textures.end(), emissionMaps.begin(), emissionMaps.end());

    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}

std::vector<std::shared_ptr<Texture>> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& variableName) {
    std::vector<std::shared_ptr<Texture>> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(dynamic_cast<Texture2D*>(textures_loaded[j].get())->path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip) {   // if texture hasn't been loaded already, load it
            auto texture2DLoaded = loadTexture2DFromFile(std::string(str.C_Str()));
            texture2DLoaded->variableName = variableName;
            textures.push_back(texture2DLoaded);
            textures_loaded.push_back(texture2DLoaded);  // store it as texture loaded for entire model, to ensure we won't unnecessarily load duplicate textures.
        }
    }
    return textures;
}

std::shared_ptr<Texture> Model::loadTexture2DFromFile(const std::string& filename) {
    std::string path = directory + '/' + filename;

    // replace '\' as '/' 替换路径中的反斜杠为正斜杠
    std::replace(path.begin(), path.end(), '\\', '/');

    return std::shared_ptr<Texture>(loadTexture2D(path));
}