package bio.shenhua;

public interface ZipFileNotifier {
    void zipStart();
    void zipProgress(int done, int total);
    void zipFinished();
}
