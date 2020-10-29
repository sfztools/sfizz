cmake --build . --target sfizz_tests --config Release -j2
.\tests\Release\sfizz_tests.exe
cmake --build . --target sfizz_lv2 --config Release -j2
cmake --build . --target sfizz_lv2_ui --config Release -j2
cmake --build . --target sfizz_vst3 --config Release -j2
