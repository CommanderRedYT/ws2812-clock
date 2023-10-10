import path from "path";

const extraDependencies = [
    [
        path.join('node_modules', 'jquery', 'dist', 'jquery.min.js'),
        'jquery.min.js',
    ],
    [
        path.join('node_modules', 'bootstrap', 'dist', 'js', 'bootstrap.bundle.min.js'),
        'bootstrap.bundle.min.js',
    ],
    [
        path.join('node_modules', 'bootstrap', 'dist', 'css', 'bootstrap.min.css'),
        'bootstrap.min.css',
    ]
];

export default extraDependencies;
