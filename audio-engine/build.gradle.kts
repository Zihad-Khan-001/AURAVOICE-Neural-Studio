plugins {
    id("com.android.library")
}

android {
    namespace = "com.auravoice.audioengine"
    compileSdk = 37

    buildFeatures {
        prefab = true
    }

    defaultConfig {
        minSdk = 27

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += "-DANDROID_STL=c++_shared"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }

        debug {
            isMinifyEnabled = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.17.0")

    implementation("com.google.oboe:oboe:1.10.0")
}
