
/* ClosestGL Example - Cube
 * 仿制云风的Mini3D：https://github.com/skywind3000/mini3d
 */

#include <ClosestGL.h>
#include <SDLInstance.h>
#include <Window.h>
#include <Mouse.h>
#include <Keyboard.h>
#include "..\SDLClasses\include\Vector4.h"

using namespace ClosestGL;

//定义基本的数据类型
using Color = Math::Vector4<float>;
using Depth = float;


//更新纹理到SDL窗口
void UpdateWindow(SDL::Window& window, Texture::Texture2D<Color>& tex)
{
	window.GetWindowSurface().Shade(
		[&tex](int x, int y, auto&, auto)
	{
		auto color = tex.AccessPixelUnsafe({ size_t(x), size_t(y) });
		return SDL::Color<uint8_t>{
			uint8_t(color.x * 255),
			uint8_t(color.y * 255),
			uint8_t(color.z * 255),
			255
		};
	});

	window.UpdateWindowSurface();
}

//顶点输入数据
struct VertexIn
{
	Math::Vector4<float> Position;
	Math::Vector2<float> TexCoord;
	Color Color;
};

constexpr std::array<VertexIn, 8> Mesh
{
	VertexIn{ { 1, -1,  1, 1 },{ 0, 0 },{ 1.0f, 0.2f, 0.2f } },
	VertexIn{ { -1, -1,  1, 1 },{ 0, 1 },{ 0.2f, 1.0f, 0.2f } },
	VertexIn{ { -1,  1,  1, 1 },{ 1, 1 },{ 0.2f, 0.2f, 1.0f } },
	VertexIn{ { 1,  1,  1, 1 },{ 1, 0 },{ 1.0f, 0.2f, 1.0f } },
	VertexIn{ { 1, -1, -1, 1 },{ 0, 0 },{ 1.0f, 1.0f, 0.2f } },
	VertexIn{ { -1, -1, -1, 1 },{ 0, 1 },{ 0.2f, 1.0f, 1.0f } },
	VertexIn{ { -1,  1, -1, 1 },{ 1, 1 },{ 1.0f, 0.3f, 0.3f } },
	VertexIn{ { 1,  1, -1, 1 },{ 1, 0 },{ 0.2f, 1.0f, 0.3f } },
};

constexpr std::array<size_t,36> Indicis
{
	0,1,2,0,3,2,
	4,5,6,4,7,6,
	0,4,5,0,1,5,
	1,5,6,1,2,6,
	2,6,7,2,3,7,
	3,7,4,3,0,4
};


//顶点输出数据
struct VertexOut
{
	Math::Vector4<float> SVPosition;
	Color Color;
	Math::Vector2<float> TexCoord;

	//透视修正器
	RenderPipeline::PerspectiveCorrector::InPixelShader<float>
		PerspectiveCorrector;

	//插值函数
	static VertexOut Lerp(float x,const VertexOut& p1, const VertexOut& p2)
	{
		return{
			Math::Lerp(x,p1.SVPosition,p2.SVPosition),
			Math::Lerp(x,p1.Color,p2.Color),
			Math::Lerp(x,p1.TexCoord,p2.TexCoord),
			Math::Lerp(x,p1.PerspectiveCorrector,p2.PerspectiveCorrector)
		};
	}
};

int main()
{
	constexpr Math::Vector2<size_t> screenSize{ 800,600 };

	//单线程执行策略
	ParallelStrategy::SingleThreadRunner runner;

	//准备棋盘纹理和采样器
	Texture::Texture2D<Color>
		texture{ {256,256} };

	texture.Shade([](auto pos) {
		bool white = false;
		if (pos.x % 32 > 16) white = !white;
		if (pos.y % 32 > 16) white = !white;
		return white ? 
			Color{ 1,1,1,1 } : 
			Color{ 63 / 255.0f,188 / 255.0f,239 / 255.0f };
	}, runner);

	Texture::Sampler::Sampler2D<
		decltype(texture),
		decltype(Texture::Sampler::UVNormalizer::UV2DClamp),
		decltype(Texture::Sampler::Fliters::Bilinear)>
		sampler(
			&texture,
			Texture::Sampler::UVNormalizer::UV2DClamp,
			Texture::Sampler::Fliters::Bilinear);


	//准备渲染目标
	Texture::Texture2D<Color> 
		colorBuffer{ screenSize };
	Texture::Texture2D<Depth>
		depthBuffer{ screenSize };

	const auto blender = 
		[](auto src, auto dst) {return src; };

	RenderPipeline::RenderTarget<1, Color, decltype(blender)>
		renderTarget{ blender,{&colorBuffer} };

	//渲染纹理方块用的渲染管线
	const auto pixelShaderFuncTex = 
		[&sampler](const VertexOut& v)
	{
		return std::array<Color, 1>
		{
			sampler.Sample(v.PerspectiveCorrector(v.TexCoord))
		};
	};

	RenderPipeline::PixelShader<decltype(renderTarget), decltype(pixelShaderFuncTex)>
		pixelShaderTex(&renderTarget, pixelShaderFuncTex);

	RenderPipeline::DepthTest<decltype(pixelShaderTex), Depth>
		depthTestTex{ &pixelShaderTex,&depthBuffer };

	RenderPipeline::TriangleRasterizer<decltype(depthTestTex), float>
		rasterTex{ &depthTestTex };

	//渲染颜色方块用的渲染管线
	const auto pixelShaderFuncColor = 
		[](const VertexOut& v)
	{
		return std::array<Color, 1>
		{
			v.Color
		};
	};

	RenderPipeline::PixelShader<decltype(renderTarget), decltype(pixelShaderFuncColor)>
		pixelShaderColor{ &renderTarget,pixelShaderFuncColor };

	RenderPipeline::DepthTest<decltype(pixelShaderColor), float>
		depthTestCol{ &pixelShaderColor,&depthBuffer };

	RenderPipeline::TriangleRasterizer<decltype(depthTestCol), float>
		rasterCol{ &depthTestCol };

	//渲染线框用的渲染管线
	const auto pixelShaderFuncWireFrame = 
		[](const auto&)
	{
		return std::array<Color, 1>
		{
			Color{0,0,0,0}
		};
	};

	RenderPipeline::PixelShader<decltype(renderTarget), decltype(pixelShaderFuncWireFrame)>
		pixelShaderWireFrame{ &renderTarget,pixelShaderFuncWireFrame };

	RenderPipeline::LineRasterizer<decltype(pixelShaderWireFrame)
	
	//准备投影矩阵
	const auto projection =
		Math::GetPerspectiveMatrixLH<float>(
			3.1415926f * 0.5f,
			screenSize.x / float(screenSize.y),
			1.0f, 
			500.0f);

	//准备视图矩阵
	auto view =
		Math::GetLookAtMatrix<float>(
			{ 3,0,0 },
			{ 1,0,0 },
			{ 0,0,1 });
	
	//准备窗体
	SDL::SDLInstance sdl;
	SDL::Window window(
		"ClosestGL Example - Cube",
		SDL::Rect<int32_t>{
			SDL::Window::Center,
			SDL::Window::Center,
			int(screenSize.x),
			int(screenSize.y)
		},
		SDL::Window::WindowFlag::Null);

	//已变换的Vertex
	std::array<VertexOut, Mesh.size()> vOut;

	//旋转角度
	float rotX = 0.35f, rotY = 0.9f;

	//鼠标设备
	SDL::Mouse mouse;
	auto mousePos = mouse.GetMouseState().position;

	//键盘设备
	SDL::Keyboard keyboard;

	enum class RenderMode : int
	{
		Texture = 0,Color = 1,WireFrame = 2
	};

	RenderMode renderMode = RenderMode::Texture;

	//主循环
	while (!sdl.QuitRequested())
	{
		//绘制背景色
		colorBuffer.Shade([screenSize](auto pos) {
			float col = 1 - pos.y / float(screenSize.y);
			return Color{ col,col,col,1 };
		}, runner);

		//清空深度缓存
		depthBuffer.Clear(0, runner);

		//世界矩阵
		const auto world = 
			Math::Matrix4<float>
			(Math::GetYRotateMatrix(rotY) * Math::Matrix3<float>(Math::GetZRotateMatrix(rotX)));

		//变换矩阵
		const auto transform = (projection * view) * world;

		//像素着色器
		Primitive::FixedTransform(
			[&transform](const VertexIn& v)
		{
			auto pos = transform * v.Position;

			RenderPipeline::PerspectiveCorrector::BeforePerspectiveDivision<float>
				uvfix(pos);

			return VertexOut {
				pos,
				uvfix(v.Color),
				uvfix(v.TexCoord),
				RenderPipeline::PerspectiveCorrector::InPixelShader<float>
					{ uvfix }
			};
		}, Mesh.data(), vOut.data(), Mesh.size(), runner);

		switch (renderMode)
		{
		case RenderMode::Color: {
			Primitive::PrimitiveListReader<3> reader{ Indicis.data(),Indicis.size() };
			rasterCol.EmitPrimitive(reader, vOut.data(), vOut.size(), runner);	//绘制色彩方块
			break;
		}
		case RenderMode::Texture: {
			Primitive::PrimitiveListReader<3> reader{ Indicis.data(),Indicis.size() };
			rasterTex.EmitPrimitive(reader, vOut.data(), vOut.size(), runner);	//绘制纹理方块
			break;
		}
		case RenderMode::WireFrame:
			break;
		}

		UpdateWindow(window, colorBuffer);

		//键盘操作
		{
			//按下Z切换渲染方式
			if(keyboard.KeyPressed("Z"))
				renderMode = RenderMode((((int)renderMode) + 1) % 3);
		}


		//鼠标操作
		{
			//移动
			auto mouseState = mouse.GetMouseState();
			if (mouseState.leftButton)
			{
				rotX -= (mouseState.position.x - mousePos.x) * 0.025f;
				rotY += (mouseState.position.y - mousePos.y) * 0.025f;
			}
			mousePos = mouseState.position;
		}
	}

	return 0;
}
