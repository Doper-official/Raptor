# Raptor model normal audit

I inspected the POSITION, NORMAL, and index accessors inside every GLB in `models/`, then compared each supplied vertex normal with the geometric normals generated from its triangles.

| Model / primitive | Vertex normals | Lengths | Alignment with triangle winding | Result |
|---|---:|---|---|---|
| `plane.glb` | 4 | all 1.0 | all corner dot products = 1.0 | Good; normals point +Y |
| `smooth_vase.glb` | 5,545 | approximately 1.0 | minimum 0.8626, median 0.9977, mean 0.9948; no negative corner comparisons | Good smooth normals; not inverted |
| `lantern.glb`, primitive 0 | 144 | approximately 1.0 | minimum approximately 1.0 | Good |
| `lantern.glb`, primitive 1 | 7,044 | approximately 1.0 | minimum 0.6857, median 1.0, mean 0.9902; no negative corner comparisons | Good smooth normals; not inverted |
| `quad.glb` | 4 | all 1.0 | all corner dot products = 1.0 | Good for its authored winding; raw normal is -Y |

No zero-length or non-finite supplied normals were found in these GLBs. The lower minimum dot products on the smooth vase and lantern are expected where smooth vertex normals span curved surfaces or meet differently oriented triangles; the overall agreement is strongly positive. I did not rewrite these asset normals.

## Important orientation notes

- `plane.glb` has four normals of `(0, +1, 0)`, so its top face is oriented upward in a Y-up world.
- `smooth_vase.glb` has a node rotation of approximately +90 degrees around X. Its mesh's long local axis is Y, so applying that node transform turns that axis toward Z. The sample scene now applies the inverse object rotation to keep the vase standing along Raptor's Y axis while still respecting the GLB transform.
- The loader now walks the active glTF scene, applies node TRS/matrix transforms recursively, and transforms normals with inverse-transpose. It also fixes triangle winding for mirrored node transforms.

## Scope

This audit covers the GLB assets shipped in this ZIP. It does not validate models that are loaded later from external files.
