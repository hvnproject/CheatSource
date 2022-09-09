#include "custom_sounds.hpp"

__forceinline void setup_sounds()
{
	CreateDirectory("csgo\\sound", nullptr);
	FILE* file = nullptr;

	file = fopen(crypt_str("csgo\\sound\\metallic.wav"), crypt_str("wb"));
	fwrite(metallic, sizeof(unsigned char), 64700, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\cod.wav"), crypt_str("wb"));
	fwrite(cod, sizeof(unsigned char), 11752, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\bubble.wav"), crypt_str("wb"));
	fwrite(bubble, sizeof(unsigned char), 41550, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\stapler.wav"), crypt_str("wb"));
	fwrite(stapler, sizeof(unsigned char), 105522, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\bell.wav"), crypt_str("wb"));
	fwrite(bell, sizeof(unsigned char), 42154, file);
	fclose(file);

	file = fopen(crypt_str("csgo\\sound\\flick.wav"), crypt_str("wb"));
	fwrite(flick, sizeof(unsigned char), 152168, file);
	fclose(file);
}