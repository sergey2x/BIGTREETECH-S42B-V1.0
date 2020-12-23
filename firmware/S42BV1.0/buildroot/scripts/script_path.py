Import("env")

# access to global construction environment
# print(env)

# Dump construction environment (for debug purpose)
# print(env.Dump())

env.Replace(LDSCRIPT_PATH="buildroot/ldscripts/STM32F030C8_FLASH.ld")

