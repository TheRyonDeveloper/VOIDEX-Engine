#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>

#include <vector>
#include <cmath>
#include <cstdint>

#define LOG_TAG "VOIDEX"

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct Vec3 {
    float x, y, z;
};

struct Vertex {
    float position[3];
    float normal[3];
};

struct Mat4 {
    float m[16] = {};
};

static GLuint shaderProgram = 0;
static GLuint VAO = 0;
static GLuint VBO = 0;
static GLuint EBO = 0;

static GLsizei indexCount = 0;
static GLint mvpLocation = -1;

static int screenWidth = 1;
static int screenHeight = 1;

static float timeValue = 0.0f;

// --------------------------------------------------
// VECTOR MATH
// --------------------------------------------------

static Vec3 subtract(Vec3 a, Vec3 b) {
    return {a.x-b.x, a.y-b.y, a.z-b.z};
}

static Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

static float dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static Vec3 normalize(Vec3 v) {
    float length = std::sqrt(dot(v, v));

    if (length < 0.00001f)
        return {0, 1, 0};

    return {
        v.x / length,
        v.y / length,
        v.z / length
    };
}

// --------------------------------------------------
// MATRIX MATH
// --------------------------------------------------

static Mat4 identity() {
    Mat4 result;

    result.m[0] = 1;
    result.m[5] = 1;
    result.m[10] = 1;
    result.m[15] = 1;

    return result;
}

static Mat4 multiply(Mat4 a, Mat4 b) {
    Mat4 result;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {

            for (int k = 0; k < 4; k++) {
                result.m[col * 4 + row] +=
                    a.m[k * 4 + row] *
                    b.m[col * 4 + k];
            }
        }
    }

    return result;
}

static Mat4 perspective(
    float fov,
    float aspect,
    float nearPlane,
    float farPlane
) {
    Mat4 result;

    float f = 1.0f / std::tan(fov * 0.5f);

    result.m[0] = f / aspect;
    result.m[5] = f;

    result.m[10] =
        (farPlane + nearPlane) /
        (nearPlane - farPlane);

    result.m[11] = -1.0f;

    result.m[14] =
        (2.0f * farPlane * nearPlane) /
        (nearPlane - farPlane);

    return result;
}

static Mat4 lookAt(
    Vec3 eye,
    Vec3 center,
    Vec3 up
) {
    Vec3 f = normalize(subtract(center, eye));
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 result = identity();

    result.m[0] = s.x;
    result.m[1] = u.x;
    result.m[2] = -f.x;

    result.m[4] = s.y;
    result.m[5] = u.y;
    result.m[6] = -f.y;

    result.m[8] = s.z;
    result.m[9] = u.z;
    result.m[10] = -f.z;

    result.m[12] = -dot(s, eye);
    result.m[13] = -dot(u, eye);
    result.m[14] = dot(f, eye);

    return result;
}

// --------------------------------------------------
// TERRAIN GENERATION
// --------------------------------------------------

static float terrainHeight(float x, float z) {

    float hills =
        std::sin(x * 0.20f) *
        std::cos(z * 0.18f) * 2.0f;

    float smallHills =
        std::sin(x * 0.48f + z * 0.2f) *
        std::cos(z * 0.35f) * 0.55f;

    return hills + smallHills;
}

static Vec3 terrainNormal(float x, float z) {

    const float e = 0.05f;

    float left = terrainHeight(x-e, z);
    float right = terrainHeight(x+e, z);

    float back = terrainHeight(x, z-e);
    float front = terrainHeight(x, z+e);

    Vec3 normal = {
        left - right,
        2.0f * e,
        back - front
    };

    return normalize(normal);
}

static void generateTerrain() {

    const int resolution = 96;
    const float worldSize = 48.0f;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(
        (resolution + 1) * (resolution + 1)
    );

    indices.reserve(
        resolution * resolution * 6
    );

    for (int z = 0; z <= resolution; z++) {

        for (int x = 0; x <= resolution; x++) {

            float px =
                ((float)x / resolution - 0.5f)
                * worldSize;

            float pz =
                ((float)z / resolution - 0.5f)
                * worldSize;

            float py = terrainHeight(px, pz);

            Vec3 normal = terrainNormal(px, pz);

            Vertex vertex = {};

            vertex.position[0] = px;
            vertex.position[1] = py;
            vertex.position[2] = pz;

            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y;
            vertex.normal[2] = normal.z;

            vertices.push_back(vertex);
        }
    }

    for (int z = 0; z < resolution; z++) {

        for (int x = 0; x < resolution; x++) {

            uint32_t a =
                z * (resolution + 1) + x;

            uint32_t b = a + 1;

            uint32_t c =
                (z + 1) * (resolution + 1) + x;

            uint32_t d = c + 1;

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(b);

            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint32_t),
        indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, position)
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, normal)
    );

    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// --------------------------------------------------
// SHADER COMPILATION
// --------------------------------------------------

static GLuint compileShader(
    GLenum type,
    const char* source
) {
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);

    glCompileShader(shader);

    GLint success = GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success) {

        char log[2048] = {};

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            nullptr,
            log
        );

        LOGE("Shader compilation failed: %s", log);

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

static void createShaders() {

    const char* vertexSource = R"(
        #version 300 es

        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 uMVP;

        out vec3 vNormal;
        out float vHeight;

        void main() {

            vNormal = normalize(aNormal);
            vHeight = aPosition.y;

            gl_Position = uMVP * vec4(aPosition, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 300 es

        precision highp float;

        in vec3 vNormal;
        in float vHeight;

        out vec4 FragColor;

        void main() {

            vec3 lightDirection =
                normalize(vec3(-0.5, 1.0, 0.3));

            float diffuse = max(
                dot(normalize(vNormal), lightDirection),
                0.0
            );

            vec3 grass = vec3(0.12, 0.30, 0.12);
            vec3 rock = vec3(0.38, 0.37, 0.34);

            float altitude = smoothstep(
                1.0,
                2.8,
                vHeight
            );

            vec3 terrainColor = mix(
                grass,
                rock,
                altitude
            );

            float lighting = 0.35 + diffuse * 0.75;

            FragColor = vec4(
                terrainColor * lighting,
                1.0
            );
        }
    )";

    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource);

    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!vertexShader || !fragmentShader) {
        return;
    }

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    GLint success = GL_FALSE;

    glGetProgramiv(
        shaderProgram,
        GL_LINK_STATUS,
        &success
    );

    if (!success) {

        char log[2048] = {};

        glGetProgramInfoLog(
            shaderProgram,
            sizeof(log),
            nullptr,
            log
        );

        LOGE("Program linking failed: %s", log);

        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (shaderProgram) {
        mvpLocation = glGetUniformLocation(
            shaderProgram,
            "uMVP"
        );
    }
}

// --------------------------------------------------
// ENGINE INITIALIZATION
// --------------------------------------------------

extern "C"
JNIEXPORT void JNICALL
Java_com_voidex_engine_MainActivity_nativeInit(
    JNIEnv* env,
    jclass clazz
) {
    (void)env;
    (void)clazz;

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(
        0.48f,
        0.68f,
        0.88f,
        1.0f
    );

    createShaders();

    generateTerrain();

    LOGE("VOIDEX Engine initialized");
}

// --------------------------------------------------
// SCREEN RESIZE
// --------------------------------------------------

extern "C"
JNIEXPORT void JNICALL
Java_com_voidex_engine_MainActivity_nativeResize(
    JNIEnv* env,
    jclass clazz,
    jint width,
    jint height
) {
    (void)env;
    (void)clazz;

    screenWidth = width > 0 ? width : 1;
    screenHeight = height > 0 ? height : 1;

    glViewport(
        0,
        0,
        screenWidth,
        screenHeight
    );
}

// --------------------------------------------------
// ENGINE RENDER LOOP
// --------------------------------------------------

extern "C"
JNIEXPORT void JNICALL
Java_com_voidex_engine_MainActivity_nativeRender(
    JNIEnv* env,
    jclass clazz
) {
    (void)env;
    (void)clazz;

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    if (!shaderProgram || !VAO || indexCount == 0) {
        return;
    }

    timeValue += 0.001f;

    float aspect =
        (float)screenWidth / (float)screenHeight;

    Mat4 projection = perspective(
        0.95f,
        aspect,
        0.1f,
        200.0f
    );

    Vec3 camera = {
        27.0f,
        18.0f,
        29.0f
    };

    Mat4 view = lookAt(
        camera,
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    );

    Mat4 mvp = multiply(projection, view);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        mvpLocation,
        1,
        GL_FALSE,
        mvp.m
    );

    glBindVertexArray(VAO);

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}
