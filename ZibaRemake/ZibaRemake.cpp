// ZibaRemake.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "ZibaRemake.h"
#include "CMagBoard.h"

using namespace std;

int main()
{
	auto magboard = CMagBoard{};
	if (FAILED(magboard.Init())) {
		std::cout << "Failed magboard init" << std::endl;
	}
	std::cout << "Initialized magboard" << std::endl;
	magboard.Reset();
	std::cout << "Reset magboard" << std::endl;

	D3DXVECTOR3* data = new D3DXVECTOR3[XSENSORS * YSENSORS];
	SENSOR_3D org[YSENSORS][XSENSORS];
	while (true) {
		const auto res = magboard.GetData(data, org);
		if (!res) continue;

		std::cout << "\033[1;1H";
		for (auto y = 0; y < YSENSORS; y++) {
			for (auto i = 0; i < 3; i++) {
				for (auto x = 0; x < XSENSORS; x++) {
					const auto v = data[y * YSENSORS + x];
					switch (i)
					{
					case 0:
						std::cout << v.x << "  ";
						break;
					case 1:
						std::cout << v.y << "  ";
						break;
					case 2:
						std::cout << v.z << "  ";
						break;
					default:
						break;
					}


					std::cout << std::endl;
				}
			}
			
		}
	}

	delete data;

	return 0;
}
