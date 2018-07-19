package com.steven.avgraphics.util;


import android.os.Build;
import android.support.annotation.RequiresApi;
import android.util.Size;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

@SuppressWarnings("WeakerAccess")
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class Camera2Helper {

    public static Size chooseOptimalSize(Size[] options, float aspectRatio, int maxWidth,
                                         int maxHeight) {
        List<Size> alternative = new ArrayList<>();
        for (Size option : options) {
            if (option.getHeight() == option.getWidth() * aspectRatio
                    && option.getWidth() <= maxWidth && option.getHeight() <= maxHeight) {
                alternative.add(option);
            }
        }

        if (alternative.size() > 0) {
            return Collections.max(alternative, new CompareSizesByArea());
        }

        return options[0];
    }

    private static class CompareSizesByArea implements Comparator<Size> {

        @Override
        public int compare(Size lhs, Size rhs) {
            return Long.signum((long) lhs.getWidth() * lhs.getHeight() - (long) rhs.getWidth() * rhs.getHeight());
        }

    }


}
