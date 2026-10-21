plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "fr.ponera.jnkwatch"
    compileSdk = 34

    defaultConfig {
        applicationId = "fr.ponera.jnkwatch"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"
    }

    // Stable signing key for distributed builds (CI). Local builds fall back to the
    // default debug key. Without a shared key, an updated APK can't install over the
    // previous one (INSTALL_FAILED_UPDATE_INCOMPATIBLE) and app data is lost.
    val ciKeystore = System.getenv("JNK_KEYSTORE")
    signingConfigs {
        if (ciKeystore != null) {
            create("shared") {
                storeFile = file(ciKeystore)
                storePassword = System.getenv("JNK_KEYSTORE_PASSWORD")
                keyAlias = System.getenv("JNK_KEY_ALIAS")
                keyPassword = System.getenv("JNK_KEY_PASSWORD")
            }
        }
    }

    buildTypes {
        debug {
            if (ciKeystore != null) signingConfig = signingConfigs.getByName("shared")
        }
        release {
            isMinifyEnabled = false
            if (ciKeystore != null) signingConfig = signingConfigs.getByName("shared")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }

    buildFeatures {
        compose = true
    }

    // Local unit tests run on the JVM: android.util.Log must no-op instead of throwing.
    testOptions {
        unitTests.isReturnDefaultValues = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation(platform("androidx.compose:compose-bom:2024.09.02"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.activity:activity-compose:1.9.2")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.8.6")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.6")
    implementation("androidx.core:core-ktx:1.13.1")

    testImplementation("junit:junit:4.13.2")
    testImplementation("org.json:json:20240303")   // real JSONObject in JVM tests
}
