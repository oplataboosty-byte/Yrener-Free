
# Включить оптимизации безопасности

# Удаление логов
-assumenosideeffects class android.util.Log {
    public static *** d(...);
    public static *** v(...);
}
# Запретить декомпиляцию через рандомизацию имен
-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable

# Удалить отладочные символы
-keepattributes Exceptions,InnerClasses,EnclosingMethod



