#include "player.h"

#define BYTES_PER_FLOAT 4
#define POSITION_COMPONENT_COUNT 2
#define TEXTURE_COORDINATES_COMPONENT_COUNT 2
#define STRIDE ((POSITION_COMPONENT_COUNT + TEXTURE_COORDINATES_COMPONENT_COUNT)*BYTES_PER_FLOAT)

/* type specifies the Shader type: GL_VERTEX_SHADER or GL_FRAGMENT_SHADER */
GLuint LoadShader(GLenum type, const char *shaderSrc) {
	GLuint shader;
	GLint compiled;

	// Create an empty shader object, which maintain the source code strings that define a shader
	shader = glCreateShader(type);

	if (shader == 0)
		return 0;

	// Replaces the source code in a shader object
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader object
	glCompileShader(shader);

	// Check the shader object compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1) {
			GLchar* infoLog = (GLchar*) malloc(sizeof(GLchar) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			LOGV("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint LoadProgram(const char *vShaderStr, const char *fShaderStr) {
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	// Create the program object
	programObject = glCreateProgram();
	if (programObject == 0)
		return 0;

	// Attaches a shader object to a program object
	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);
	// Bind vPosition to attribute 0
	glBindAttribLocation(programObject, 0, "vPosition");
	// Link the program object
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked) {
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1) {
			GLchar* infoLog = (GLchar*) malloc(sizeof(GLchar) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			LOGV("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return GL_FALSE;
	}

	// Free no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

int CreateProgram() {
	GLuint programObject;

	GLbyte vShaderStr[] = "attribute vec4 a_Position;  			\n"
			"attribute vec2 a_TextureCoordinates;   			\n"
			"varying vec2 v_TextureCoordinates;     			\n"
			"void main()                            			\n"
			"{                                      			\n"
			"    v_TextureCoordinates = a_TextureCoordinates;   \n"
			"    gl_Position = a_Position;    					\n"
			"}                                      			\n";

	GLbyte fShaderStr[] =
			"precision mediump float; \n"
					"uniform sampler2D u_TextureUnit;                	\n"
					"varying vec2 v_TextureCoordinates;              	\n"
					"void main()                                     	\n"
					"{                                               	\n"
					"    gl_FragColor = texture2D(u_TextureUnit, v_TextureCoordinates);  \n"
					"}                                               	\n";

	// Load the shaders and get a linked program object
	programObject = LoadProgram((const char*) vShaderStr,
			(const char*) fShaderStr);
	if (programObject == 0) {
		return GL_FALSE;
	}

	// Store the program object
	global_context.glProgram = programObject;

	// Get the attribute locations
	global_context.positionLoc = glGetAttribLocation(programObject,
			"v_position");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glGenTextures(1, &global_context.mTextureID);
	glBindTexture(GL_TEXTURE_2D, global_context.mTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	return 0;
}

void setUniforms(int uTextureUnitLocation, int textureId) {
	// Pass the matrix into the shader program.
	//glUniformMatrix4fv(uMatrixLocation, 1, false, matrix);

	// Set the active texture unit to texture unit 0.
	glActiveTexture(GL_TEXTURE0);

	// Bind the texture to this unit.
	glBindTexture(GL_TEXTURE_2D, textureId);

	// Tell the texture uniform sampler to use this texture in the shader by
	// telling it to read from texture unit 0.
	glUniform1i(uTextureUnitLocation, 0);
}

void Render(uint8_t *pixel) {
	GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
			0.0f };

	// Set the viewport
	glViewport(0, 0, global_context.vcodec_ctx->width,
			global_context.vcodec_ctx->height);

	// Clear the color buffer
	//glClear(GL_COLOR_BUFFER_BIT);

	// Use the program object
	glUseProgram(global_context.glProgram);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global_context.vcodec_ctx->width,
			global_context.vcodec_ctx->height, 0, GL_RGB,
			GL_UNSIGNED_SHORT_5_6_5, pixel);

	// Retrieve uniform locations for the shader program.
	GLint uTextureUnitLocation = glGetUniformLocation(global_context.glProgram,
			"u_TextureUnit");
	setUniforms(uTextureUnitLocation, global_context.mTextureID);

	// Retrieve attribute locations for the shader program.
	GLint aPositionLocation = glGetAttribLocation(global_context.glProgram,
			"a_Position");
	GLint aTextureCoordinatesLocation = glGetAttribLocation(
			global_context.glProgram, "a_TextureCoordinates");

	// Order of coordinates: X, Y, S, T
	// Triangle Fan
	GLfloat VERTEX_DATA[] = { 0.0f, 0.0f, 0.5f, 0.5f, -1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
			0.0f, -1.0f, -1.0f, 0.0f, 1.0f };

	glVertexAttribPointer(aPositionLocation, POSITION_COMPONENT_COUNT, GL_FLOAT,
			false, STRIDE, VERTEX_DATA);
	glEnableVertexAttribArray(aPositionLocation);

	glVertexAttribPointer(aTextureCoordinatesLocation, POSITION_COMPONENT_COUNT,
			GL_FLOAT, false, STRIDE, &VERTEX_DATA[POSITION_COMPONENT_COUNT]);
	glEnableVertexAttribArray(aTextureCoordinatesLocation);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	eglSwapBuffers(global_context.eglDisplay, global_context.eglSurface);
}
