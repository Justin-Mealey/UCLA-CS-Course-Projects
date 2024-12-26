
# Assignment 2: Geometric transformations with matrices.

In this assignment, the goals are:
1. Understand how shapes are represented in Three.js (and most 3D engines) by manually creating a cube geometry. (2.5pts)
2. Implement a set of geometric transformations (translation, rotation, scaling) using model transformation matrices. (12.5pts)
3. Make the transformations depend on time, which will allow us to create a swinging effect animation. (2.5pts)
4. Draw the outline of the cube instead of the faces. (2.5pts)
5. Add user input via pressing specific keys. We will switch between wireframe and solid view of the cubes. (5pts)
   


### Data structure of a cube
We will start from a very similar point to the one we used in the previous assignment: a single cube. The main difference is that we will manually create a box geometry to better understand how geometry is represented in Three.js (and most graphics software).
A geometry is mainly defined by two components:
1. `positions`. List of 3D vertices, with no notion of how they are connected.
2. `indices`. How are the vertices connected, by defining triangular faces formed by groups of 3 vertices.


If you the run the code as is, you should be able to see the front and left face of a cube. If you cannot see the left face, make sure you are looking at it from the left side by mouse dragging. This is due to backface culling, something we will learn about soon.

Next we break down what the code is doing so far.

We start by defining the positions of the vertices. There are different ways of doing it in Three.js, but we will use a Float32Array where every group of 3 consecutive elements will later be interpreted as a 3D point (a vertex).
````javascript
const l = 0.5; // We define a constant l, which is HALF the length of the side of the cube.
const positions = new Float32Array([
    // Front face
    -l, -l,  l, // 0
     l, -l,  l, // 1
     l,  l,  l, // 2
    -l,  l,  l, // 3

    // Left face
    -l, -l, -l, // 4
    -l, -l,  l, // 5
    -l,  l,  l, // 6 
    -l,  l, -l, // 7
  ]);
````

You might be wondering: Why are we defining the top left and bottom left vertices twice? (Vertices 0,3 are the same as 5,7). It has to do with normals. Normals in 3D graphics engines are not defined per face but per vertex. Maybe this is counterintuitive, since we usually think about normals as "the perpendicular vector to a surface" and does not make that much sense to define for a vertex. We will learn why this is the case in Assignment 3. For now, just know we need to define a single normal per each vertex. The particular geometry of a cube has one specific issue: each vertex (of a cube) has 3 different normals, one for each of the faces where it belongs. The most common solution is, we define each vertex multiple times, each one with a different normal value, corresponging the each face. Note that in a full cube, each vertex needs to be defined 3 times, one for each face, with a different normal value.

Here we define the normals:
We will assign one normal to each vertex that we defined in the `positions` array. They will be assigned in the same order as we assigned the `positions` list, that's how the engine knows which normal corresponds to which vertex.
````javascript
 const normals = new Float32Array([
    // Front face
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
  
    // Left face
    -1, 0, 0,
    -1, 0, 0,
    -1, 0, 0,
    -1, 0, 0,
  ]);
  ````

Even though in the previous code snippet we group them by face for better readability, the `positions` or `normals` array have no actual notion of connectivity between vertices. The next step is to define this connectivity with triangle faces. We do this with the indices array, which will group the vertices in groups of three, each forming a triangle surface. Vertices are indexed from `0..23` in the same order of appearance in the `positions` list. Then, every group of three consecutive integers will represent a triangle. For example, the first triangle `0, 1, 2`is formed by the points `[(-l, -l,  l), (l, -l,  l),
     (l,  l,  l,)]` and the second triangle `0, 2, 3` is formed of `[(-l, -l,  l),
     (l,  l,  l),
    (-l,  l,  l)]`. The two combined comprise a full square face. Each square face is defined by two triangles, so we need 6 indices per face and 36 indices in total. Fill in the rest of the indices array
````javascript
const indices = [
    // Front face
    0, 1, 2,
    0, 2, 3,
  
    // Left face
    4, 5, 6,
    4, 6, 7,
  ];
````
Now we can create a custom `THREE.BufferGeometry()` object and set the attributes of the geometry to the ones we defined.

```javascript
const custom_cube_geometry = new THREE.BufferGeometry();
custom_cube_geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3)); // setting the position attribute
custom_cube_geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3)); // setting the normal attribute
custom_cube_geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(indices), 1)); // setting the index attribute
```

and draw the cube using 
````javascript
//let cube = new THREE.Mesh( geometry, phong_material );
let cube = new THREE.Mesh( custom_cube_geometry, phong_material );
scene.add(cube);
````

**Exercise 1 (2.5 points): Fill in the rest of positions, normals and vertices to form a full cube.** Hint: You should have `8*3=24` vertices in the positions list, `8*3=24` vertices in the normal list and `6*2=12` triangles or `12*3=36` indices in the indices list. If you cannot see some of the triangles you defined, make sure you are following the right hand rule when defining the indices.

### Stacking cubes
We will now stack cubes on top of each other. For this, we will instanciate multiple cubes and vertically translate each one of them. In order to create multiple cubes, we do not need to create multiple `THREE.BoxGeometry()` nor `THREE.MeshBasicMaterial()` objects. We can reuse the same geometry and material objects for all the cubes. We will need to instanciate a new `THREE.Mesh()` object for each cube.

We do not want to just `add()` the cubes to the scene but also "keep track" of them in order to later apply transformations. We can do this by creating an array `cubes` and pushing (appending) each cube to it.

```javascript
let cubes = [];
for (let i = 0; i < 7; i++) {
	let cube = new THREE.Mesh(geometry, material);
	cubes.push(cube);
	scene.add(cube);
}
```

Now we have 7 cubes but they are all placed in the origin by default, so you will only see one.

As you learned (or are about to learn) in the lectures, transformations in 3D graphics are implemented by multiplying 4x4 transformation matrices to 3D points represented in homogeneous coordinates. 
There are different ways to apply transformations in Three.js. A high level way of doing so is setting it via `cube.{position/scale/rotationX/rotationY/rotationZ}.set(...)`. What this does under the hood is to build the corresponding 4x4 transformation matrices and setting the model transform matrix accordingly. For this assignment, we will be manually building and setting these 4x4 model transformation matrices instead of using Three.js' high level API.

For this, we first need to tell Three.js not to overwrite the model transformation matrix. If we do not do this, the model will be transformed according to the mesh's `position/scale/rotationX/rotationY/rotationZ` properties and not according to our manually defined transformation matrices. We can do this by setting the `matrixAutoUpdate` property of the cube to `false`:

```javascript
let cubes = [];
for (let i = 0; i < 7; i++) {
	let cube = new THREE.Mesh(geometry, material);
	cube.matrixAutoUpdate = false;
	cubes.push(cube);
	scene.add(cube);
}
```

### Transformations

We will now implement the cube stack via vertical translation.
As you know from the lectures, the translation matrix is defined as:

```
| 1 0 0 tx |
| 0 1 0 ty |
| 0 0 1 tz |
| 0 0 0 1  |
```
We will create a function that builds a translation matrix given the `(tx, ty, tz)` translation values.

```javascript
function translationMatrix(tx, ty, tz) {
	return new THREE.Matrix4().set(
		1, 0, 0, tx,
		0, 1, 0, ty,
		0, 0, 1, tz,
		0, 0, 0, 1
	);
}
```

Now there are different ways of "stacking cubes one on top of another" using this matrix. For reasons you will understand later in this assignment, we will do it by 1) building a single translation matrix that translates the cube in the vertical direction by `2*l` (length of cube side), and 2) iteratively multiplying this matrix with itself `i` times, where `i` in `[0, ... N_cubes-1]` is the index of the cube in the array. This way, the first cube will be translated by `0*2l = 0` (at the origin), the second cube will be translated by `1*2l` units in the y direction, the third cube will be translated by `2*2l` in the vertical direction, and so on, forming a contiguous stack of cubes.
```javascript
// Instanciate the cubes
let cubes = [];
for (let i = 0; i < 7; i++) {
	let cube = new THREE.Mesh(geometry, material);
	cube.matrixAutoUpdate = false;
	cubes.push(cube);
	scene.add(cube);
}
// Translate the cubes.
const translation = translationMatrix(0, 2*l, 0); // Translate 2l units in the y direction
let model_transformation = new THREE.Matrix4(); // model transformation matrix we will update
for (let i = 0; i < cubes.length; i++) {
	cubes[i].setMatrix(model_transformation);
    model_transformation.multiplyMatrices(translation, model_transformation);
}
```

We could have done the instantiation and translation in a single for loop. In later parts of the assignments, you will understand why we want to separate it.

### Tilting the cubes.

We will now tilt the cubes to the left by a fixed angle. An important requirement for this assignment is that **the top left corner of each cube has to touch the bottom left corner of the cube above it**. 
Of course, we will use a rotation matrix for this. In particular, because we want to rotate the cubes around the z axis, we will use the following rotation matrix:

```
| cos(theta) -sin(theta) 0 0 |
| sin(theta)  cos(theta) 0 0 |
|     0           0      1 0 |
|     0           0      0 1 |
```	
where `theta` is the angle of rotation. We will create a function that builds a rotation matrix given the angle of rotation. Fill in the following function:

```javascript
function rotationMatrixZ(theta) {
	return new THREE.Matrix4().set(
        Math.cos(theta), -1 * Math.sin(theta), 0, 0,
        Math.sin(theta), Math.cos(theta), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
	);
}
```

**Exercise 2 (5 points): Tilt each cubes 20 degrees to the left with respect to the previous cube in the stack. Make sure the top left corner of each cube is in perfect contact to the bottom left corner of the next cube.** Look at discussion slides for hints.

### Scaling the cubes
We will now change the height of the cube to side 1.5 instead of 1.0. The other two sides (width and depth) still need to be length 1. You cannot change the geometric definition of the cube, you need to do it via scaling transformations. First fill in the scaling matrix function:

```javascript
function scalingMatrix(sx, sy, sz) {
  return new THREE.Matrix4().set(
    sx, 0,  0,  0,
    0,  sy, 0,  0,
    0,  0,  sz, 0,
    0,  0,  0,  1

  );
}
```

**Exercise 3 (5 points): Make the cubes taller using the scaling matrix transform.** Make sure that the faces of the cuboid are still perpendicular to each other.

### Making the cubes swing

So far we made the cubes tilt by a fixed a amount. Now we want to make them swing back and forth. We will do this by making the angle a function of time instead of tiling by a constant MAX_ANGLE amount,. 

For this, we will first move the code that transforms the cubes into the `animate()` function. This is a special function that is called at every frame, as opposed to the code that is only executed once. It will allow us to transform the cubes as a function of time.

Note that we are using the Clocl class to measure the time elapsed between frames and adding it to a variable that measures the absolute time since the beginning of the animation.

```javascript
let animation_time = 0;
let delta_animation_time;
let rotation_angle;
const clock = new THREE.Clock();

MAX_ANGLE = 20 * Math.PI/360 // 20 degrees converted to radians
T = 3 // oscilation persiod in seconds

function animate() {
    // ...
    delta_animation_time = clock.getDelta();
    animation_time += delta_animation_time; 
    // ...
    // feel free to add other auxiliary variables if needed.
    rotation_angle = // make it a function of animation_time

    const rotation = rotationMatrixZ(rotation_angle);
    // ...
    // Define other matrices
    
    // Apply transformations
    for (let i = 0; i < cubes.length; i++) {
	    // cubes[i].setMatrix(...);
        // model_transformation.multiplyMatrices(...);
    }
```


**Exercise 4 (2.5 points)** Make the cube swing to the left and back to the center. The swinging needs to be smooth, and in particular it has to follow in a sinusoidal curve that goes from 0 to 20 degrees with a total period of oscilation of 3 seconds. The cubes must never swing right.



### Wireframe view
We will now learn how to draw the outline or edges of the cube, instead of the faces. As you can imagine, this will require a fundamentally different approach to the way we initially drew the cube, since we cannot draw an outline (lines) based on triangles (surfaces). In order to do this, we will use a `LineSegments` object which will replace the `Mesh` object. We will also pass it a `BufferGeometry` but this time it will be interpreted and rendered very differently. The new `BufferGeometry` will also have a `positions` attribute. However, this time the `positions` attribute will consist of pairs of 3D points, each of which will represent a line segment. 

````javascript
const wireframe_vertices = new Float32Array([
    // Front face
        -l, -l, l,
        l, -l, l,
        l, -l, l,
        l, l, l,
        l, l, l,
        -l, l, l,
        -l, l, l,
        -l, -l, l,
    // Top face
        -l, l, -l,
        -l, l, l,
        -l, l, l,
        l, l, l,
        l, l, l,
        l, l, -l,
]);

const wireframe_greometry = new THREE.BufferGeometry();
wireframe_greometry.setAttribute( 'position', new THREE.BufferAttribute( wireframe_vertices, 3 ) );

const line = new THREE.LineSegments( geometry );
````

**Exercise 5 : Finish the wirefreme cube, and replace the Mesh() cube by the new LineSegments cube**. All you need to do is finish the definition of the `wireframe_vertices` array and replace the `Mesh` object we defined earier by the `LineSegments` object as shown above. The model transformations and the rest of the code should look the same.

### Freezing the swing with user keyboard input
We want the cubes to stop swinging when the user presses the `s` key. We want them to stop at the maximum angle of swing. 

We will first add an event listener, which will keep track of keys being pressed, and will automatically call somes function if any key is pressed. This function we define will toggle a variabe called `still` which will be used to determine if the cubes should stop swinging or not. 

```javascript
let still = false;
window.addEventListener('keydown', onKeyPress); // onKeyPress is called each time a key is pressed
// Function to handle keypress
function onKeyPress(event) {
    switch (event.key) {
        case 's': // Note we only do this if s is pressed.
            still = !still;
            break;
        default:
            console.log(`Key ${event.key} pressed`);
    }
}
```

Now we use the `still` variable to stop the swinging of the cubes when the `s` key is pressed.


### Switching between wireframe and solid view

**Exercise 6 (5 points): Switch between Wireframe and Mesh view when the w key is pressed**. Hint: One way to do it is to first create an array `cubes` with 7 `Mesh` cubes and another array `cubes_wireframe` with 7 `LineSegments`. You can toggle the `visible` property of each of the cubes when the `w` key is pressed. For example: `cubes[i].visible = !cubes[i].visible;`. Make sure that if cubes[i].visible are `true`, then you have set cubes_wireframe[i] to `false`, and vice-versa. Add another entry to the `onKeyPress` function to toggle between the two views with letter `w`. 



# Submission instructions.

1. Remove the `node_modules`, `package-lock.json`
2. Zip the `Assignment2` folder.
3. Upload to BruinLearn.
4. If the file is too large, you probably did something wrong. Make sure you only zip the necessary files.


