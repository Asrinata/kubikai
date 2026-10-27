plugins { id("com.android.application") }

android {
    namespace = "com.asrin.kubik"
    compileSdk = 34
    buildToolsVersion = "34.0.0"

    defaultConfig {
        applicationId = "com.asrin.kubik"
        minSdk = 26
        targetSdk = 34
        versionCode = 12
        versionName = "1.6.5"
    }


    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    lint { checkReleaseBuilds = false; abortOnError = false }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
