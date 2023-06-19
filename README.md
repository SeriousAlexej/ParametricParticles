# ParametricParticles

Advanced particle entity for the *Serious Sam - The Second Encounter 1.07*

<img width="322" height="258" align="left" src="./Images/Particles_Rain.gif">
<img width="322" height="258" align="left" src="./Images/Particles_FireSmoke.gif">
<img width="322" height="258" align="left" src="./Images/Particles_TreeBirds.gif">
<img width="322" height="258" align="left" src="./Images/Particles_Beams.gif">
<img width="322" height="258" align="left" src="./Images/Particles_MovingShower.gif">
<img width="322" height="258" src="./Images/Particles_AnimatedSprites.gif">

---

1. [Introduction](#intro)
2. [Installation](#install)
3. [New entities](#entities)
   1. [ParametricParticles](#particles)

---

# Introduction<a name="intro"></a>

This is a collection of a few new entities, that are handy during mapping.

With customizable particles, you can add special effects and a chef's touch to your map!

Waterfalls, smoke, sparkles, clouds, fire, ambient particles, dusty air, fireflies and much more - your imagination is the limit!

# Installation<a name="install"></a>

1. Download the archive from the [latest release](https://github.com/SeriousAlexej/ParametricParticles/releases/latest) page.
2. Extract the archive to the root directory of *Serious Sam - The Second Encounter 1.07*
3. Launch **Serious Editor** and add all new classes to the virtual tree.
<img src="./Images/Particles_Install.gif">


# New entities<a name="entities"></a>

Particles are implemented with 6 new entities.

<img src="./Images/Entities.png">

To quickly check them out and see some examples, you can startup a **Parametric Particles - Demo Level** level from within the game.

<img src="./Images/DemoLevel.png">

### ParametricParticles<a name="particles"></a>
<img src="./Images/ParametricParticlesTbn.png">

This is the main entity, that defines all parameters of the particles.

#### Gist
When this entity is **Active**, it spawnes between **Spawn count min** and **Spawn count max** particles every **Spawn interval (seconds)** seconds.
Each particle is defined by a set of properties, starting from the *Particle* word - like **Particle color**.
Initial particle position is determined by a **Spawner shape** entity, or, if not specified, is taken from the current entity's location.

#### Events

Following events can be sent to this entity:

* **Activate event** - this event activates the entity, so that new particles will be spawned.
* **Deactivate event** - this event stops new particles from spawning, but *existing* particles will continue to exist (until they expire).
* **Stop event** - this event stops new particles from spawning and kills all alive particles.

#### Properties

* **Active** - if *TRUE*, new particles are spawned each **Spawn interval (seconds)**.
* **Background** - if *TRUE*, particles will be rendered as a part of a background. This is useful when making clouds or flying birds etc.
* **Blend type** - defines the blending mode of particle's texture on screen.
* **Clipping box size X (for rendering only)** - this property defines the stretch of this entity, change it only if you experience problems with clipping (i.e. particles disappear sooner than they should when camera is turned away from them).
* **Clipping box size Y** - same as above, but for *Y* axis.
* **Clipping box size Z** - same as above, but for *Z* axis.
* **Flat type**
* **Online Help...**
* **Particle alpha...**
* **Particle color**
* **Particle lifetime max**
* **Particle lifetime min**
* **Particle placement**
* **Particle rotation force (chained)**
* **Particle rotation max**
* **Particle rotation min**
* **Particle size uniform**
* **Particle size X max**
* **Particle size X min**
* **Particle size Y max**
* **Particle size Y min**
* **Particle stretch X...**
* **Particle stretch Y...**
* **Particle velocity force (chained)**
* **Spawn count max**
* **Spawn count min**
* **Spawn interval (seconds)**
* **Spawner shape**
* **Texture**
* **Texture animation**
* **Texture animation FPS**
* **Texture columns**
* **Texture rows**
* **Texture tiles count**
* **Texture type**

